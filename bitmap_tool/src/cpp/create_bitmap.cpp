#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <map>

// A simple 5x7 pixel bitmap font for a few characters
const std::map<char, std::vector<uint8_t>> font = {
    {'A', {0x7E, 0x09, 0x09, 0x09, 0x7E}},
    {'B', {0x7F, 0x49, 0x49, 0x49, 0x36}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
    {'D', {0x7F, 0x41, 0x41, 0x41, 0x3E}},
    {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
    {'F', {0x7F, 0x09, 0x09, 0x01, 0x01}},
    {'G', {0x3E, 0x41, 0x49, 0x49, 0x3A}},
    {'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
    {'I', {0x41, 0x41, 0x7F, 0x41, 0x41}},
    {'J', {0x20, 0x40, 0x41, 0x3F, 0x01}},
    {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
    {'L', {0x7F, 0x40, 0x40, 0x40, 0x40}},
    {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
    {'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
    {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
    {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
    {'S', {0x26, 0x49, 0x49, 0x49, 0x32}},
    {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}},
    {'W', {0x3F, 0x40, 0x38, 0x40, 0x3F}},
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
    {'0', {0x3E, 0x45, 0x49, 0x51, 0x3E}},
    {'1', {0x21, 0x21, 0x7F, 0x01, 0x01}},
    {'2', {0x21, 0x43, 0x45, 0x49, 0x31}},
    {'3', {0x42, 0x41, 0x51, 0x69, 0x46}},
    {'4', {0x0F, 0x08, 0x08, 0x7F, 0x08}}
};

/**
 * @brief Sets a pixel in a 1-bit, bottom-to-top, row-major pixel buffer.
 * @param buffer The pixel buffer to modify.
 * @param width The width of the image.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 */
void set_pixel_bmp(std::vector<uint8_t>& buffer, int width, int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= 48) return;
    int inverted_y = 47 - y;
    int byte_index = (inverted_y * (width / 8)) + (x / 8);
    int bit_index = 7 - (x % 8);
    if (byte_index < buffer.size()) {
        buffer[byte_index] |= (1 << bit_index);
    }
}

/**
 * @brief Draws text onto the pixel buffer using the hardcoded font.
 * @param buffer The pixel buffer to modify.
 * @param text The string to draw.
 * @param start_x The starting x-coordinate.
 * @param start_y The starting y-coordinate.
 */
void draw_text(std::vector<uint8_t>& buffer, const std::string& text, int start_x, int start_y) {
    int current_x = start_x;
    for (char c : text) {
        char upper_c = toupper(c); // Use uppercase for font lookup
        if (font.count(upper_c)) {
            const auto& char_data = font.at(upper_c);
            for (int col = 0; col < 5; ++col) { // Character width is 5
                for (int row = 0; row < 8; ++row) { // Character height is 8 (standard byte)
                    if ((char_data[col] >> row) & 1) {
                        set_pixel_bmp(buffer, 160, current_x + col, start_y + row);
                    }
                }
            }
            current_x += 6; // Move to the next character position (5px width + 1px spacing)
        }
    }
}

int main(int argc, char* argv[]) {
    // --- MODIFICATION: Argument Parsing ---
    // Check if the correct number of arguments is provided.
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <output_filename.bmp> \"Text to display\"" << std::endl;
        return 1; // Return an error code
    }

    std::string filename = argv[1];
    std::string text_to_draw = argv[2];

    const int width = 160;
    const int height = 48;
    const int bits_per_pixel = 1;
    const int row_stride = ((width * bits_per_pixel + 31) / 32) * 4;
    const int pixel_data_size = row_stride * height;
    const int color_table_size = 8;
    const int file_header_size = 14;
    const int info_header_size = 40;
    const int file_size = file_header_size + info_header_size + color_table_size + pixel_data_size;
    
    // --- Create the pixel data ---
    std::vector<uint8_t> pixel_buffer(pixel_data_size, 0x00); // Initialize to black

    // --- MODIFICATION: Auto-center the text ---
    int char_width_with_spacing = 6;
    int text_pixel_width = text_to_draw.length() * char_width_with_spacing - 1;
    int start_x = (width - text_pixel_width) / 2;
    int start_y = (height - 8) / 2; // Center vertically (assuming 8px font height)
    if (start_x < 0) start_x = 0; // Prevent negative start position for long text

    draw_text(pixel_buffer, text_to_draw, start_x, start_y);

    // --- Create the BMP file (no changes from here on) ---
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Could not open file for writing: " << filename << std::endl;
        return 1;
    }

    // -- File Header (14 bytes) --
    file.put('B').put('M');
    file.write(reinterpret_cast<const char*>(&file_size), 4);
    file.write("\0\0\0\0", 4); // Reserved
    uint32_t pixel_offset = file_header_size + info_header_size + color_table_size;
    file.write(reinterpret_cast<const char*>(&pixel_offset), 4);

    // -- Info Header (40 bytes) --
    file.write(reinterpret_cast<const char*>(&info_header_size), 4);
    file.write(reinterpret_cast<const char*>(&width), 4);
    file.write(reinterpret_cast<const char*>(&height), 4);
    uint16_t planes = 1;
    file.write(reinterpret_cast<const char*>(&planes), 2);
    uint16_t bpp = bits_per_pixel;
    file.write(reinterpret_cast<const char*>(&bpp), 2);
    file.write("\0\0\0\0", 4); // No compression
    file.write(reinterpret_cast<const char*>(&pixel_data_size), 4);
    file.write("\0\0\0\0", 4); // Print resolution X (unused)
    file.write("\0\0\0\0", 4); // Print resolution Y (unused)
    uint32_t colors_in_palette = 2;
    file.write(reinterpret_cast<const char*>(&colors_in_palette), 4);
    file.write("\0\0\0\0", 4); // All colors are important

    // -- Color Table (Palette) --
    file.write("\0\0\0\0", 4); // Black
    file.write("\xFF\xFF\xFF\0", 4); // White

    // -- Pixel Data --
    file.write(reinterpret_cast<const char*>(pixel_buffer.data()), pixel_buffer.size());

    file.close();

    std::cout << "Successfully created " << filename << std::endl;

    return 0;
}