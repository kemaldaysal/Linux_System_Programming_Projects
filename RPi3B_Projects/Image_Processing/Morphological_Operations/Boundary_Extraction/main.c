/* ------ Notes Section
1) Boundary Extraction
Option A: (A + B) - A (Dilation - self)
or
Option B: A - (A-B) (self - Erosion)
--- Potential Improvements
1) Make it able to read image inputs and conver them to image matrixes.

*/

// ---- Libraries ----
#include <stdint.h>

// ---- Enums ----

// ---- Function Prototypes ----
void rgb2gray_u8(const uint8_t *img_rgb, uint8_t *img_gray, const unsigned long w, const unsigned long h);

enum RGB_Locations
{
    R_loc = 0,
    G_loc = 1,
    B_loc = 2
};

int main()
{
    // ----- Settings
    const bool B_kernel[3][3] = {{0, 1, 0},
                    {1, 1, 1},
                    {0, 1, 0}};

    const uint8_t img_org[5][5] = {{0, 0, 0, 0, 0},
                          {0, 1, 0, 1, 0},
                          {0, 0, 0, 0, 0},
                          {0, 1, 0, 1, 0},
                          {0, 0, 0, 0, 0}};

    unsigned long w = sizeof(img_org[0]); unsigned long l = sizeof(img_org[1]);

    // img_org = imread("./Image_Inputs/img_boundary_extraction.png"); // CONVERT THIS TO C

    if (sizeof(img_org[2] == 3)) // if image matrix contains R,G,B, convert it to grayscale first.
    {
        uint8_t img_gray[w][l];
        rgb2gray_u8(img_org, img_gray, w, l);
    }

    return 0;
}

void rgb2gray_u8(const uint8_t *img_rgb, uint8_t *img_gray, const unsigned long w, const unsigned long h)
{
    const unsigned long n = w * h;

    for (int i = 0; i < n; i++) {
        uint8_t R = img_rgb[3*i + 0];
        uint8_t G = img_rgb[3*i + 1];
        uint8_t B = img_rgb[3*i + 2];

        // Y ≈ (77R + 150G + 29B) / 256
        img_gray[i] = (uint8_t)((77u*R + 150u*G + 29u*B) >> 8);
    }    
    // uint8_t img_grayscale[len_col][len_row]; // USE MALLOC HERE!!
    // int test = sizeof(*img_rgb);
    // // malloc(sizeof(*img_rgb))

    // for (unsigned long i_cols = 0; i_cols < len_col; i_cols++)
    // {
    //     for (unsigned long i_rows = 0; i_rows < len_row; i_rows++)
    //     {
    //         // uint8_t R = img_rgb[i_cols][i_rows][R_loc]; // Change it later, this is wrong!!
    //         // uint8_t G = img_rgb[i_cols][i_rows][G_loc]; // Change it later, this is wrong!!
    //         // uint8_t B = img_rgb[i_cols][i_rows][B_loc]; // Change it later, this is wrong!!

    //         // img_grayscale[i_rows][i_cols] = ((77 * R) + (150 * G) + (29 * B)) >> 8;
    //     }
    // }
}