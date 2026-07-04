#include <cairo/cairo.h>
#include <stdio.h>
#include <stdlib.h>

// Make sure to define implementation in exactly one file
#define CLAY_IMPLEMENTATION 
#include "../../src/renderer/clay_renderer_cairo.c"

#define WIDTH  1920 // Adjusted to match your bar's width
#define HEIGHT 30   // Adjusted to match your bar's height
#define RGBA(r,g,b,a) (struct wb_widget_color){ (r), (g), (b), (a) }

int main(void)
{
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t *c = cairo_create(surface);

    cairo_set_source_rgba(c, 0.95, 0.95, 0.95, 1);
    cairo_paint(c);

    char *fonts[] = {
        "Quicksand Semibold",
        "Callistoga",
        NULL
    };

    // --- CLAY INITIALIZATION ---
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(clayMemory, (Clay_Dimensions) {WIDTH, HEIGHT}, (Clay_ErrorHandler) { 0 });
	Clay_Cairo_Initialize(c);
	Clay_SetMeasureTextFunction(Clay_Cairo_MeasureText, fonts);


    // --- LAYOUT DEFINITION ---
    
	Clay_BeginLayout();

// MAIN-0
    CLAY(CLAY_ID("main-0"), {
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_FIXED(WIDTH),
                .height = CLAY_SIZING_FIXED(HEIGHT)
            },
            .childAlignment = {
                // BUG: Centering the parent on the X-axis overrides child GROW sizing.
                // Change this to CLAY_ALIGN_X_LEFT to fix group-1's width!
                .x = CLAY_ALIGN_X_CENTER, 
                .y = CLAY_ALIGN_Y_CENTER
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
        .backgroundColor = (Clay_Color){40, 40, 40, 255}
    }){
        
        // GROUP-0 (FIT)
        CLAY(CLAY_ID("group-0"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIT(0),
                    .height = CLAY_SIZING_FIT(0)
                }
            },
            .backgroundColor = (Clay_Color){255, 100, 100, 255}
        }){
            // Dummy block to give group-0 a 48x19 size so it doesn't collapse
        }

        // GROUP-1 (GROW)
        CLAY(CLAY_ID("group-1"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0)
                }
            },
            .backgroundColor = (Clay_Color){100, 255, 100, 255}
        }){}

        // GROUP-2 (FIT)
        CLAY(CLAY_ID("group-2"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIT(0),
                    .height = CLAY_SIZING_GROW(0)
                }
            },
            .backgroundColor = (Clay_Color){100, 100, 255, 255}
        }){
            // Dummy block to give group-2 a 48x19 size
            CLAY(CLAY_ID("dummy-2"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(48),
                        .height = CLAY_SIZING_FIXED(19)
                    }
                }
            }){}
        }

    } // End main-0

    Clay_RenderCommandArray renderCommands = Clay_EndLayout();


	Clay_Cairo_Render(renderCommands, fonts);
    cairo_surface_write_to_png(surface, "clay_cairo_test.png");

    cairo_destroy(c);
    cairo_surface_destroy(surface);

    puts("generated clay_cairo_test.png");
    return 0;
}
