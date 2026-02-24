// начала переписывать всё заново полностью под C++ с использованием структуры вектор
#include <iostream>
#include <stdio.h>
#include <vector>
#include <complex.h>
#include <stdio.h>
#include <cmath>
#include <string.h>
#include "signal_processing.h" 
#include <chrono>
#include <thread>

#include "imgui.h"
#include "implot.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"

using namespace std;

void run_gui(
    const vector <int>& bits, 
    const vector<complex<float>>& IQ_bpsk,
    const vector<complex<float>>& IQ_upsampled, 
    const vector<complex<float>>& IQ_convolved, 
    const vector<complex<float>>& IQ_convolved2,
    const vector<complex<float>>& IQ_true,
    const vector<int> bits2,
    const vector<float>& erof,
    int samples_per_symbol
){
    // 1) Инициализация SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow(
        "Vizualization", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 768, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    // 2) Инициализация контекста Dear Imgui
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Ввод\вывод
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Включить Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Включить Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Включить Docking

    // 2.1) Привязка Imgui к SDL2 и OpenGl backend'ам
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    while (running) {
        // 3.0) Обработка event'ов (inputs, window resize, mouse moving, etc.);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            cout << "Processing some event: "<< event.type << " timestamp: " << event.motion.timestamp << endl;
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // 3.1) Начинаем создавать новый фрейм;
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_None);

        if (ImGui::Begin("Original bits")){

            vector<float> bit_floats;
            bit_floats.resize(bits.size());
            for (size_t i = 0; i < bits.size(); ++i) {
                bit_floats[i] = bits[i];
            }

            if (ImPlot::BeginPlot("Original bits")){
                ImPlot::SetupAxes("Bit index", "Value" );
                ImPlot::PlotScatter("Bits", bit_floats.data(), bit_floats.size(), 1.0, 0.0);
                ImPlot::EndPlot();
            }

            ImGui::StyleColorsClassic();
        }
        ImGui::End();

        if (ImGui::Begin("I/Q")){

            static vector<float> re, im;
            re.resize(IQ_bpsk.size()); im.resize(IQ_bpsk.size());
            for (size_t i = 0; i < IQ_bpsk.size(); ++i) {
                re[i] = IQ_bpsk[i].real();
                im[i] = IQ_bpsk[i].imag();
            }

            if (ImPlot::BeginPlot("I/Q")){
                ImPlot::SetupAxes("I", "Q");
                ImPlot::PlotScatter("Symbols", re.data(), im.data(), re.size());
                ImPlot::EndPlot();
            }

            ImGui::StyleColorsClassic();
        }
        ImGui::End();

        if (ImGui::Begin("Upsampling")){

            static vector<float> time_up, re_up, im_up;
                int N = IQ_upsampled.size();
                time_up.resize(N); re_up.resize(N); im_up.resize(N);
                for (int i = 0; i < N; ++i) {
                    time_up[i] = static_cast<float>(i);
                    re_up[i] = IQ_upsampled[i].real();
                    im_up[i] = IQ_upsampled[i].imag();
                }

            if (ImPlot::BeginPlot("Upsampling")){
                ImPlot::SetupAxes("Sample index", "Amplitude");
                ImPlot::PlotLine("Real", time_up.data(), re_up.data(), N);
                ImPlot::PlotLine("Imag", time_up.data(), im_up.data(), N);
                ImPlot::EndPlot();
            }

            ImGui::StyleColorsClassic();
        }
        ImGui::End();

        if (ImGui::Begin("Convolve")){

            static vector<float> time_c, re_c, im_c;
            int N = IQ_convolved.size();
            time_c.resize(N); re_c.resize(N); im_c.resize(N);
            for (int i = 0; i < N; ++i) {
                time_c[i] = static_cast<float>(i);
                re_c[i] = IQ_convolved[i].real();
                im_c[i] = IQ_convolved[i].imag();
            }

            if (ImPlot::BeginPlot("Convolve")){
                ImPlot::SetupAxes("Sample index", "Amplitude");
                ImPlot::PlotLine("Real", time_c.data(), re_c.data(), N);
                ImPlot::PlotLine("Imag", time_c.data(), im_c.data(), N);
                ImPlot::EndPlot();
            }

            ImGui::StyleColorsClassic();
        }
        ImGui::End();

        if (ImGui::Begin("Matched filter")){

            vector<float> time_c, re_c, im_c;
            int N = IQ_convolved2.size();
            time_c.resize(N); re_c.resize(N); im_c.resize(N);
            for (int i = 0; i < N; ++i) {
                time_c[i] = static_cast<float>(i);
                re_c[i] = IQ_convolved2[i].real();
                im_c[i] = IQ_convolved2[i].imag();
            }

            if (ImPlot::BeginPlot("Matched filter")){
                ImPlot::SetupAxes("Sample index", "Amplitude");
                ImPlot::PlotLine("Real", time_c.data(), re_c.data(), N);
                ImPlot::PlotLine("Imag", time_c.data(), im_c.data(), N);
                for (size_t i = 0; i < erof.size(); ++i) {
                    int idx = (int)erof[i];

                    float x_data[] = {(float)idx, (float)idx};
                    float y_data[] = {-20.0f, 20.0f};
                        
                    ImPlot::PlotLine("##Sync", x_data, y_data, 2);
                    
                }
                ImPlot::EndPlot();
            }
            ImGui::StyleColorsClassic();
        }
        ImGui::End();

        if (ImGui::Begin("Downsampling")){

            static vector<float> time_up, re_up, im_up;
                int N = IQ_true.size();
                time_up.resize(N); re_up.resize(N); im_up.resize(N);
                for (int i = 0; i < N; ++i) {
                    time_up[i] = static_cast<float>(i);
                    re_up[i] = IQ_true[i].real();
                    im_up[i] = IQ_true[i].imag();
                }

            if (ImPlot::BeginPlot("Downsampling")){
                ImPlot::SetupAxes("Sample index", "Amplitude");
                ImPlot::PlotLine("Real", time_up.data(), re_up.data(), N);
                ImPlot::PlotLine("Imag", time_up.data(), im_up.data(), N);
                ImPlot::EndPlot();
            }

            ImGui::StyleColorsClassic();
        }
        ImGui::End();

        if (ImGui::Begin("I/Q received")){

            if (ImPlot::BeginPlot("I/Q received")) {
                vector<float> re(IQ_true.size()), im(IQ_true.size());
            
                for (size_t i = 0; i < IQ_true.size(); ++i) {
                    re[i] = IQ_true[i].real();
                    im[i] = IQ_true[i].imag();
                }

        
                ImPlot::PlotScatter("Received", re.data(), im.data(), re.size());
                
                ImPlot::EndPlot();
            }
        

        }
        ImGui::End();

        if (ImGui::Begin("Received bits")){

            vector<float> bit_floats;
            bit_floats.resize(bits2.size());
            for (size_t i = 0; i < bits2.size(); ++i) {
                bit_floats[i] = bits2[i];
            }

            if (ImPlot::BeginPlot("Received bits")){
                ImPlot::SetupAxes("Bit index", "Value" );
                ImPlot::PlotScatter("Bits", bit_floats.data(), bit_floats.size(), 1.0, 0.0);
                ImPlot::EndPlot();
            }
        

            ImGui::StyleColorsClassic();
        }
        ImGui::End();


        // 3.3) Отправляем на рендер;
        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // 4) Закрываем приложение безопасно.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(){

    const int num_bits = 10;
    const int samples_per_symbol = 10;
    vector<int> bits;
    bits.resize(num_bits);
    
    cout << "Оригинальные биты" <<endl;

    for (int i = 0; i < num_bits; i++){
        bits[i] = rand() % 2;
        cout << bits[i] << " ";
    }

    cout << endl;
    cout << "Комплексные значения" <<endl;

    vector<complex<float>> IQ_bpsk = to_bpsk(bits);

    for (int i = 0; i < num_bits; i++){
        cout << IQ_bpsk[i] << " ";
    }
    
    cout << endl;
    cout << "После апсемплинга" <<endl;

    vector<complex<float>> IQ_upsampled = upsampling(IQ_bpsk, num_bits, samples_per_symbol);

    for (int i = 0; i < (int)IQ_upsampled.size(); i++){
        cout << IQ_upsampled[i] << " ";
    }
    cout << endl;
    cout << "После свертки" <<endl;

    vector<complex<float>> IQ_convolved = convolve(IQ_upsampled, samples_per_symbol);

    for (int i = 0; i < (int)IQ_convolved.size(); i++){
        cout << IQ_convolved[i] << " ";
    }
    cout << endl;
    cout << "После повторной свертки" <<endl;

    vector<complex<float>> IQ_convolved2 = convolve(IQ_convolved, samples_per_symbol);

    for (int i = 0; i < (int)IQ_convolved2.size(); i++){
        cout << IQ_convolved2[i] << " ";
    }
    cout << endl;

    vector<float> erof = symbol_sync(IQ_convolved2, samples_per_symbol);

    cout << "Массив индексов отсчетов правильных символов" <<endl;
    for (int i = 0; i < (int)erof.size(); i++){
        cout << erof[i] << " ";
    }
    cout << endl;

    vector<complex<float>> IQ_true = downsampling(erof, IQ_convolved2);

    cout << "Правильные сэмплы" <<endl;
    for (int i = 0; i < (int)IQ_true.size(); i++){
        cout << IQ_true[i] << " ";
    }
    cout << endl;

    vector<int> bits2 = from_bpsk(IQ_true);

    cout << "Полученные биты" <<endl;
    for (int i = 0; i < (int)bits2.size(); i++){
        cout << bits2[i] << " ";
    }
    cout << endl;

    run_gui(bits, IQ_bpsk, IQ_upsampled, IQ_convolved, IQ_convolved2, IQ_true, bits2, erof, samples_per_symbol);
    return 0;
  
}