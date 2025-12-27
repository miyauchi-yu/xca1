#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuiFileDialog.h>
#include <thread>
#include <atomic>
#include <fstream>
#include <filesystem>
#include "logger/logger.hpp"
#include "analysis/analysis.hpp"
#include "imgui_spinner.hpp"

static std::atomic<bool> g_isAnalyzing{false};
static std::string g_resultPassword;
static std::thread g_analysisThread;
static const char* LAST_DIR_FILE = "dir";

void drawFileDialog();
void executeAnalysisAsync(const std::string& selectedFile, int maxPasswordLen);
std::string loadLastDirectory();
void saveLastDirectory(const std::string& dir);
void drawCenteredBigPassword(const std::string& password);

/*
 * メイン関数
 */
int main() {
    // 開始ログ
    Logger::getInstance().log(LogLevel::Info, "Application started");

    // GLFW初期化
    if (!glfwInit()){
        Logger::getInstance().log(LogLevel::Error, "Failed to initialize GLFW");
        return 1;
    }

    // OpenGL 3.3, Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ウィンドウサイズを指定
    const int window_width = 1280;
    const int window_height = 720;

    // ウィンドウ作成
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "ZIP File Password Analysis v1.0", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    // モニター情報を取得（メインモニター）
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (mode){
        int xpos = (mode->width - window_width) / 2;
        int ypos = (mode->height - window_height) / 2;
        glfwSetWindowPos(window, xpos, ypos);
    }

    // ウィンドウ作成
    // GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "ImGui + GLFW + OpenGL3 Example", monitor, nullptr);
    // if (window == nullptr)
    //    return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Syncを有効にする

    // ImGui初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGui::StyleColorsDark();

    glfwMakeContextCurrent(window);

    // メインループ
    while (!glfwWindowShouldClose(window)){
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        ImGui::Begin(
                "Password Analysis Window",
                nullptr,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings);

        // ファイル選択ダイアログの描画
        drawFileDialog();

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // クリーンアップ
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    // 終了ログ
    Logger::getInstance().log(LogLevel::Info, "Application stopped");

    return 0;
}

/*
 * ファイル選択ダイアログ表示
 */
void drawFileDialog() {
    static std::string selectedFile;
    static std::string lastDir = loadLastDirectory();
    static int maxPasswordLen;
    static std::string analyzedPassword;

    // 最大パスワード長を設定
    ImGui::SliderInt("Max Length", &maxPasswordLen, 1, 16);
    if (maxPasswordLen < 1) maxPasswordLen = 1;

    // ボタンでファイルダイアログを開く
    if (ImGui::Button("File Open")) {
        IGFD::FileDialogConfig config;
        config.path = lastDir;
        ImGuiFileDialog::Instance()->OpenDialog("FileOpenDialog", "File Open", ".zip", config);
    }
    ImGui::SameLine();

    // ダイアログ描画
    if (ImGuiFileDialog::Instance()->Display("FileOpenDialog")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            // OKが押された場合のみパスをセット
            selectedFile = ImGuiFileDialog::Instance()->GetFilePathName();
            lastDir = ImGuiFileDialog::Instance()->GetCurrentPath();
            saveLastDirectory(lastDir);
        } else {
            // キャンセルされた場合は空にする
            selectedFile.clear();
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (!selectedFile.empty()){
        ImGui::Text("File Path: %s", selectedFile.c_str());
        ImGui::SameLine();

        if (ImGui::Button("Analyze Passwords") && !g_isAnalyzing.load()) {
            Logger::getInstance().log(LogLevel::Info, "Analyzing passwords from file: " + selectedFile);

            g_isAnalyzing.store(true);
            g_resultPassword.clear();

            g_analysisThread = std::thread(executeAnalysisAsync, selectedFile, maxPasswordLen);
            g_analysisThread.detach(); // UIブロック防止
        }
        if (g_isAnalyzing.load()) {
            ImGui::Text("Analyzing password...");
            ImGui::SameLine();
            Spinner("##spinner", 8.0f, 3);
            ImGui::SameLine();
        }
    }

    if (!g_isAnalyzing.load() && !g_resultPassword.empty())
        drawCenteredBigPassword(g_resultPassword);
}

/*
 * パスワード解析実行(暗号方式がZIPCryptoの数字 + 英字 + 記号のみ対応)
 */
void executeAnalysisAsync(const std::string& selectedFile, int maxPasswordLen) {
    Analysis analysis(selectedFile, maxPasswordLen);
    g_resultPassword = analysis.run();
    g_isAnalyzing.store(false);
}

/*
 * 最後に開いたディレクトリを読み込む
 */
std::string loadLastDirectory() {
    std::ifstream ifs(LAST_DIR_FILE);
    std::string dir;

    if (ifs && std::getline(ifs, dir)) {
        if (std::filesystem::exists(dir))
            return dir;
    }

    return ".";
}

/*
 * 最後に開いたディレクトリを保存する
 */
void saveLastDirectory(const std::string& dir) {
    std::ofstream ofs(LAST_DIR_FILE, std::ios::trunc);
    if (ofs)
        ofs << dir;
}

/*
 * 解析成功したパスワードを画面中央に大きく表示する
 */
void drawCenteredBigPassword(const std::string& password) {
    ImGuiIO& io = ImGui::GetIO();

    // フォントを大きく
    ImGui::SetWindowFontScale(4.0f); // ← 倍率（2.5〜4.0）

    ImVec2 textSize = ImGui::CalcTextSize(password.c_str());

    // 画面中央に配置
    ImGui::SetCursorPos(ImVec2(
        (io.DisplaySize.x - textSize.x) * 0.5f,
        (io.DisplaySize.y - textSize.y) * 0.5f
    ));

    ImGui::TextColored(
        ImVec4(0.2f, 1.0f, 0.2f, 1.0f), // 成功感のある緑
        "%s",
        password.c_str()
    );

    ImGui::SetWindowFontScale(1.0f); // 元に戻す
}
