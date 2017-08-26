#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <cstdint>

#include "SDL2/SDL.h"
#include "djc_math/djc_math.hpp"


struct window_spec {
    window_spec(int dpi_unscaled_width, int dpi_unscaled_height, Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE) noexcept(false)
    :   sdl_window{nullptr}
    ,   sdl_renderer{nullptr} 
    ,   sdl_render_texture{nullptr}
    ,   x{SDL_WINDOWPOS_CENTERED}
    ,   y{SDL_WINDOWPOS_CENTERED}
    ,   dpi_unscaled_width{dpi_unscaled_width}
    ,   dpi_unscaled_height{dpi_unscaled_height}
    ,   dpi_scaled_width{dpi_unscaled_width}
    ,   dpi_scaled_height{dpi_unscaled_height}
    ,   renderer_width{dpi_unscaled_height}
    ,   renderer_height{dpi_unscaled_height}
    ,   flags{flags}
    {
        if (SDL_GetDisplayDPI(0, nullptr, &dpi_system_current, nullptr) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could'nt get display DPI %s", SDL_GetError());
            dpi_system_current = dpi_system_default;
        }

        recalculate_and_set_dpi_scaling(dpi_system_default, dpi_system_current); 

        if ((sdl_window = SDL_CreateWindow("PerlinAnimation", x, y, dpi_scaled_width, dpi_scaled_height, flags)) == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
            throw "SDL_CreateWindow() failed"; 
        }

        if ((sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED)) == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
            throw "SDL_CreateRenderer() failed";
        }

        if (SDL_GetRendererOutputSize(sdl_renderer, &renderer_width, &renderer_height) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
            throw "SDL_GetRendeerOutputSize() failed"; 
        }

        if((sdl_render_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, renderer_width, renderer_height)) == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
            throw "SDL_CreteTexture() failed";
        }
        
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
        SDL_RenderClear(sdl_renderer);   
    }

    ~window_spec() {
         SDL_RenderClear(sdl_renderer);
         SDL_DestroyRenderer(sdl_renderer);
         SDL_DestroyTexture(sdl_render_texture);
         SDL_DestroyWindow(sdl_window);
    } 

    void recalculate_and_set_dpi_scaling(float dpi_default, float dpi_current) {
        dpi_scaled_width = static_cast<int>(dpi_unscaled_width * dpi_current / dpi_default);
        dpi_scaled_height = static_cast<int>(dpi_unscaled_height * dpi_current / dpi_default);
    }

    SDL_Window *sdl_window;
    SDL_Renderer *sdl_renderer;
    SDL_Texture *sdl_render_texture;
    int x;
    int y;
    int dpi_unscaled_width;
    int dpi_unscaled_height; 
    int dpi_scaled_width;
    int dpi_scaled_height;
    int renderer_width;
    int renderer_height;
    Uint32 flags ; 

#if defined(__APPLE__)
    static constexpr const float dpi_system_default = 72.0f;
#elif defined(_WIN32)
    static constexpr const float dpi_system_default = 96.0f;
#else
    static_assert(false, "dpi_system_default not defined for current system");
#endif
    
    float dpi_system_current {dpi_system_default};
};

int main(int argc, char *argv[]) {
    
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL Could not be initialised: %s", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
    try {
        window_spec main_window{200, 200};
           
        djc::math::perlin<float> noisy(227);
        
        std::vector<std::uint32_t> pixel_buffer(main_window.renderer_width * main_window.renderer_height, 0);

        SDL_Event event;
        double zstep = 0.0;
        bool running = true;

        while(running) {

            // check for input events 
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    SDL_Log("SDL_QUIT");
                    running = false;
                    break; 
                }
            }

            // draw perlin background
            for (auto y = 0; y < main_window.renderer_height; y++) {
                for (auto x = 0; x < main_window.renderer_width; x++) {
                    double X = (double)x / (double)main_window.renderer_width;
                    double Y = (double)y / (double)main_window.renderer_height;
                  
                    std::uint8_t noise = noisy.noise(X * 10 ,Y * 10 , zstep) * 255; 
                    
                    pixel_buffer[main_window.renderer_width * y + x] = (255 << 24) + (noise << 16) + (noise << 8) + noise; 
                }
            }

            zstep+= 0.01f;
            
            // end render
            {
                SDL_UpdateTexture(main_window.sdl_render_texture, NULL, pixel_buffer.data(), sizeof(std::uint32_t) * main_window.renderer_width); //Copy entire array only once
                SDL_RenderClear(main_window.sdl_renderer);
                SDL_RenderCopy(main_window.sdl_renderer, main_window.sdl_render_texture, NULL, NULL);
                SDL_RenderPresent(main_window.sdl_renderer);
            }
        }
    } catch (const char *msg) {
        std::cout << msg << std::endl;
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Quit();
    return EXIT_SUCCESS;
}
