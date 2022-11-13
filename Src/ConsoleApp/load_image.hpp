#pragma once
#include <filesystem>
#include <simde/x86/avx.h>
#include <simde/x86/avx2.h>
#include <stb_image.h>
#include <boost/scope_exit.hpp>
#include "../HookDll/image.h"

#include "exceptions.hpp"
uint32_t swap_channel_and_mix_alpha(uint32_t uc) {
	//UC:ARGB
	uint8_t* channels = reinterpret_cast<uint8_t*>(&uc);

	auto t = channels[0];
	channels[0] = channels[2];
	channels[2] = t;

	channels[0] = (channels[3] * channels[0]) / 255;
	channels[1] = (channels[3] * channels[1]) / 255;
	channels[2] = (channels[3] * channels[2]) / 255;
	return uc;
}
inline void swap_channel_and_mix_alpha(uint32_t* data, size_t pixel) noexcept
{
	//Mostly taken from https://github.com/Wizermil/premultiply_alpha
	while (pixel && (reinterpret_cast<uintptr_t>(data) & 31)) {
		*data = swap_channel_and_mix_alpha(*data);
		data++;
		pixel--;
	}
	while (pixel & 3) {
		pixel--;
		data[pixel] = swap_channel_and_mix_alpha(data[pixel]);
	}
	if (pixel == 0)return;
	//assert((reinterpret_cast<uintptr_t>(data) & 31) == 0);
	size_t const max_simd_pixel = pixel * sizeof(uint32_t) / sizeof(simde__m256i);

	simde__m256i const mask_alpha_color_odd_255 = simde_mm256_set1_epi32(static_cast<int>(0xff000000));
	simde__m256i const div_255 = simde_mm256_set1_epi16(static_cast<short>(0x8081));

	simde__m256i const mask_shuffle_alpha = simde_mm256_set_epi32(0x0f800f80, 0x0b800b80, 0x07800780, 0x03800380, 0x0f800f80, 0x0b800b80, 0x07800780, 0x03800380);
	simde__m256i const mask_shuffle_color_odd = simde_mm256_set_epi32(static_cast<int>(0x80800d80), static_cast<int>(0x80800980), static_cast<int>(0x80800580), static_cast<int>(0x80800180), static_cast<int>(0x80800d80), static_cast<int>(0x80800980), static_cast<int>(0x80800580), static_cast<int>(0x80800180));

	simde__m256i const mask_shuffle_color_even = simde_mm256_set_epi32(static_cast<int>(0x0C800E80), static_cast<int>(0x08800A80), static_cast<int>(0x04800680), static_cast<int>(0x00800280), static_cast<int>(0x0C800E80), static_cast<int>(0x08800A80), static_cast<int>(0x04800680), static_cast<int>(0x00800280));

	//Swap red and blue.

	simde__m256i color, alpha, color_even, color_odd;

	for (simde__m256i* ptr = reinterpret_cast<simde__m256i*>(data), *end = ptr + max_simd_pixel; ptr != end; ++ptr) {
		color = simde_mm256_load_si256(ptr);

		alpha = simde_mm256_shuffle_epi8(color, mask_shuffle_alpha);

		color_even = simde_mm256_shuffle_epi8(color, mask_shuffle_color_even);

		color_odd = simde_mm256_shuffle_epi8(color, mask_shuffle_color_odd);
		color_odd = simde_mm256_or_si256(color_odd, mask_alpha_color_odd_255);

		color_odd = simde_mm256_mulhi_epu16(color_odd, alpha);
		color_even = simde_mm256_mulhi_epu16(color_even, alpha);

		color_odd = simde_mm256_srli_epi16(simde_mm256_mulhi_epu16(color_odd, div_255), 7);
		color_even = simde_mm256_srli_epi16(simde_mm256_mulhi_epu16(color_even, div_255), 7);

		color = simde_mm256_or_si256(color_even, simde_mm256_slli_epi16(color_odd, 8));

		simde_mm256_store_si256(ptr, color);
	}
}
inline HANDLE load_img(const std::filesystem::path& img, uint32_t mode, uint32_t background) {
	if (img.empty())return NULL;

	//1.load image into memory
	FILE* f = _wfopen(img.wstring().c_str(), L"rb");
	my_assert(f != NULL, "Cannot open image to read");
	int width, height, channels;
	stbi_uc* content = stbi_load_from_file(f, &width, &height, &channels, 4);
	my_assert(content, "Cannot load image");
	
	BOOST_SCOPE_EXIT(content) { free(content); }BOOST_SCOPE_EXIT_END;


	//2.swap channel and pre-multiply alpha
	swap_channel_and_mix_alpha(reinterpret_cast<uint32_t*>(content), width * height);

	//3.construct map view
	size_t required_length = sizeof(ImageHeader) + width * height * 4;
	LARGE_INTEGER len;
	len.QuadPart = static_cast<LONGLONG>(required_length);
	HANDLE hMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, len.HighPart, len.LowPart, NULL);
	expect_win32(hMapping != NULL && hMapping != INVALID_HANDLE_VALUE);
	LPVOID data = MapViewOfFile(hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	expect_win32(data != NULL);
	
	//4.construct struct to pass to child
	ImageHeader* ret_header = reinterpret_cast<ImageHeader*>(data);
	void* ret_content = ret_header + 1;
	ret_header->mode = mode;
	ret_header->width = width;
	ret_header->height = height;
	ret_header->background = background;
	memcpy(ret_content, content, width * height * 4);
	UnmapViewOfFile(data);
	return hMapping;
}
