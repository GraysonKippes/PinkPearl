#ifndef IMAGE_H
#define IMAGE_H

typedef unsigned char byte_t;

typedef struct {
	
	// The data of this image.
	byte_t *m_data;

	// The width of this image, in pixels.
	size_t m_width;

	// The height of this image, in pixels.
	size_t m_height;

	// The number of components in each pixel.
	// 1 = Grayscale;
	// 2 = Grayscale and Alpha;
	// 3 = Red, Green, Blue; and
	// 4 = Red, Green, Blue, and Alpha.
	size_t m_num_channels;

} image_t;

// Loads an image at the given filepath.
image_t load_image(const char *path);

// Frees an image, destroying the data buffer.
void free_image(image_t image);

#endif	// IMAGE_H
