import sys
from PIL import Image
import os
import time


def image_to_crgb_matrix(image_path, output_file):
    # Open the image
    img = Image.open(image_path)

    # Check if the image is 18x32
    if img.size != (18, 32):
        raise ValueError("Image must be 18x32 pixels")

    # Get the pixel data
    pixels = img.load()

    # Create the matrix name from the image filename
    image_filename = os.path.basename(image_path)
    matrix_name, _ = os.path.splitext(image_filename)

    # Write matrix definition to the output file
    with open(output_file, 'a') as f:
        f.write(f'const CRGB {matrix_name}[32][18] = {{\n')

        for y in range(32):
            f.write('    {')
            for x in range(18):
                r, g, b = pixels[x, y][:3]  # Ignore the alpha channel if it exists
                f.write(f'CRGB({r}, {g}, {b})')
                if x < 17:
                    f.write(', ')
            f.write('}')
            if y < 31:
                f.write(',\n')
            else:
                f.write('\n')

        f.write('};\n\n')


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python image_to_crgb_matrix.py <image_filenames...>")
        sys.exit(1)

    output_file = 'image_matrices.h'

    # Create or overwrite the output file
    with open(output_file, 'w') as f:
        f.write('#ifndef IMAGE_MATRICES_H\n')
        f.write('#define IMAGE_MATRICES_H\n\n')
        f.write('#include <FastLED.h>\n\n')  # Directly include FastLED.h

    image_filenames = sys.argv[1:]

    for image_filename in image_filenames:
        image_path = os.path.abspath(image_filename)
        try:
            image_to_crgb_matrix(image_path, output_file)
            print(f"Generated matrix for {image_filename}")
        except Exception as e:
            print(f"Error processing {image_filename}: {str(e)}")

    # Append end of include guard to the output file
    with open(output_file, 'a') as f:
        f.write('#endif // IMAGE_MATRICES_H\n')
