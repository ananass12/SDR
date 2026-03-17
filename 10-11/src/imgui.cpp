#include "imgui_implot.h"

void run_gui(
    const vector <int>& bits, 
    const vector<complex<float>>& IQ_bpsk,
    const vector<complex<float>>& IQ_upsampled, 
    const vector<complex<float>>& IQ_convolved, 
    const vector<complex<float>>& rx_subset,
    const vector<complex<float>>& IQ_matched,
    const vector<complex<float>>& IQ_symbol_sync,
    const vector<complex<float>>& IQ_freq,
    const vector<complex<float>>& znach_cor,
    const vector<complex<float>>& data_only,
    const vector<int> bits2,
    int samples_per_symbol
){
    // === Инициализация ===
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow("SDR", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    while (running) {
        // Обработка событий
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_None);

        // ВЕРХНЯЯ ПОЛОВИНА
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("##TopPanel", nullptr, 
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            
            if (ImGui::BeginTabBar("TopTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
                
                if (ImGui::BeginTabItem("Original bits")) {
                    vector<float> bit_floats(bits.size());
                    for (size_t i = 0; i < bits.size(); ++i) bit_floats[i] = static_cast<float>(bits[i]);
                    if (ImPlot::BeginPlot("##BitsPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Bit index", "Value");
                        ImPlot::PlotScatter("Bits", bit_floats.data(), bit_floats.size(), 1.0, 0.0);
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("I/Q")) {
                    vector<float> re(IQ_bpsk.size()), im(IQ_bpsk.size());
                    for (size_t i = 0; i < IQ_bpsk.size(); ++i) {
                        re[i] = IQ_bpsk[i].real();
                        im[i] = IQ_bpsk[i].imag();
                    }
                    if (ImPlot::BeginPlot("##IQPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("I", "Q");
                        ImPlot::SetupAxisLimits(ImAxis_Y1, -2, 2, ImGuiCond_Always);
                        ImPlot::PlotScatter("Symbols", re.data(), im.data(), re.size());
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Upsampling")) {
                    int N = IQ_upsampled.size();
                    vector<float> t(N), re(N), im(N);
                    for (int i = 0; i < N; ++i) {
                        t[i] = static_cast<float>(i);
                        re[i] = IQ_upsampled[i].real();
                        im[i] = IQ_upsampled[i].imag();
                    }
                    if (ImPlot::BeginPlot("##UpsamplePlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Sample index", "Amplitude");
                        ImPlot::PlotLine("Real", t.data(), re.data(), N);
                        ImPlot::PlotLine("Imag", t.data(), im.data(), N);
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Convolve")) {
                    int N = IQ_convolved.size();
                    vector<float> t(N), re(N), im(N);
                    for (int i = 0; i < N; ++i) {
                        t[i] = static_cast<float>(i);
                        re[i] = IQ_convolved[i].real();
                        im[i] = IQ_convolved[i].imag();
                    }
                    if (ImPlot::BeginPlot("##ConvPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Sample index", "Amplitude");
                        ImPlot::PlotLine("Real", t.data(), re.data(), N);
                        ImPlot::PlotLine("Imag", t.data(), im.data(), N);
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // НИЖНЯЯ ПОЛОВИНА
        ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("##BottomPanel", nullptr, 
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            
            if (ImGui::BeginTabBar("BottomTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
                
                if (ImGui::BeginTabItem("Received signal")) {
                    int N = rx_subset.size();
                    vector<float> t(N), re(N), im(N);
                    for (int i = 0; i < N; ++i) {
                        t[i] = static_cast<float>(i);
                        re[i] = rx_subset[i].real();
                        im[i] = rx_subset[i].imag();
                    }
                    if (ImPlot::BeginPlot("##SignalPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Sample index", "Amplitude");
                        ImPlot::PlotLine("Real", t.data(), re.data(), N);
                        ImPlot::PlotLine("Imag", t.data(), im.data(), N);
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Matched filter")) {
                    int N = IQ_matched.size();
                    vector<float> t(N), re(N), im(N);
                    for (int i = 0; i < N; ++i) {
                        t[i] = static_cast<float>(i);
                        re[i] = IQ_matched[i].real();
                        im[i] = IQ_matched[i].imag();
                    }
                    if (ImPlot::BeginPlot("##MatchedPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Sample index", "Amplitude");
                        ImPlot::PlotLine("Real", t.data(), re.data(), N);
                        ImPlot::PlotLine("Imag", t.data(), im.data(), N);
                        
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("I/Q after matched")) {
                    if (ImPlot::BeginPlot("##IQMatchedPlot", ImVec2(-1,-1))) {
                        vector<float> re(IQ_matched.size()), im(IQ_matched.size());
                        for (size_t i = 0; i < IQ_matched.size(); ++i) {
                            re[i] = IQ_matched[i].real();
                            im[i] = IQ_matched[i].imag();
                        }
                        ImPlot::SetupAxes("I", "Q");
                        ImPlot::PlotScatter("", re.data(), im.data(), re.size());
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }

                
                if (ImGui::BeginTabItem("Symbol sync")) {
                    int N = IQ_symbol_sync.size();
                    vector<float> t(N), re(N), im(N);
                    for (int i = 0; i < N; ++i) {
                        t[i] = static_cast<float>(i);
                        re[i] = IQ_symbol_sync[i].real();
                        im[i] = IQ_symbol_sync[i].imag();
                    }
                    if (ImPlot::BeginPlot("##DownPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Sample index", "Amplitude");
                        ImPlot::PlotLine("Real", t.data(), re.data(), N);
                        ImPlot::PlotLine("Imag", t.data(), im.data(), N);
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("I/Q after down")) {
                    if (ImPlot::BeginPlot("##IQDownPlot", ImVec2(-1,-1))) {
                        vector<float> re(IQ_symbol_sync.size()), im(IQ_symbol_sync.size());
                        for (size_t i = 0; i < IQ_symbol_sync.size(); ++i) {
                            re[i] = IQ_symbol_sync[i].real();
                            im[i] = IQ_symbol_sync[i].imag();
                        }
                        ImPlot::SetupAxes("I", "Q");
                        ImPlot::PlotScatter("", re.data(), im.data(), re.size());
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Frequency sync")) {
                    int N = IQ_freq.size();
                    vector<float> t(N), re(N), im(N);
                    for (int i = 0; i < N; ++i) {
                        t[i] = static_cast<float>(i);
                        re[i] = IQ_freq[i].real();
                        im[i] = IQ_freq[i].imag();
                    }
                    if (ImPlot::BeginPlot("##FreqPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Sample index", "Amplitude");
                        ImPlot::PlotLine("Real", t.data(), re.data(), N);
                        ImPlot::PlotLine("Imag", t.data(), im.data(), N);
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("I/Q after freq")) {
                    if (ImPlot::BeginPlot("##IQFreqPlot", ImVec2(-1,-1))) {
                        vector<float> re(IQ_freq.size()), im(IQ_freq.size());
                        for (size_t i = 0; i < IQ_freq.size(); ++i) {
                            re[i] = IQ_freq[i].real();
                            im[i] = IQ_freq[i].imag();
                        }
                        ImPlot::SetupAxes("I", "Q");
                        ImPlot::PlotScatter("", re.data(), im.data(), re.size());
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Values correlation")) {
                    int N = znach_cor.size();
                    vector<float> t(N), val(N);
                    for (int i = 0; i < N; ++i) {
                        t[i] = static_cast<float>(i);
                        val[i] = abs(znach_cor[i]); 
                    }
                    if (ImPlot::BeginPlot("##CorrPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Lag (sample index)", "Correlation magnitude");
                        ImPlot::PlotLine("Value correlation", t.data(), val.data(), N);
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Without Barker code")) {
                    int N = data_only.size();
                    vector<float> t(N), re(N), im(N);
                    for (int i = 0; i < N; ++i) {
                        t[i] = static_cast<float>(i);
                        re[i] = data_only[i].real();
                        im[i] = data_only[i].imag();
                    }
                    if (ImPlot::BeginPlot("##DataPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Sample index", "Amplitude");
                        ImPlot::PlotLine("Real", t.data(), re.data(), N);
                        ImPlot::PlotLine("Imag", t.data(), im.data(), N);
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("I/Q without barker code")) {
                    if (ImPlot::BeginPlot("##IQDataPlot", ImVec2(-1,-1))) {
                        vector<float> re(data_only.size()), im(data_only.size());
                        for (size_t i = 0; i < data_only.size(); ++i) {
                            re[i] = data_only[i].real();
                            im[i] = data_only[i].imag();
                        }
                        ImPlot::SetupAxes("I", "Q");
                        ImPlot::PlotScatter("", re.data(), im.data(), re.size());
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Received bits")) {
                    vector<float> bf(bits2.size());
                    for (size_t i = 0; i < bits2.size(); ++i) bf[i] = static_cast<float>(bits2[i]);
                    if (ImPlot::BeginPlot("##RecvBitsPlot", ImVec2(-1,-1))) {
                        ImPlot::SetupAxes("Bit index", "Value");
                        ImPlot::PlotScatter("Bits", bf.data(), bf.size(), 1.0, 0.0);
                        ImPlot::EndPlot();
                    }
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}