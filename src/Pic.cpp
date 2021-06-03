#include "FileTypes.hpp"

#include <stdint.h>

#include <fstream>
#include "Config.hpp"
#include "FS.hpp"
#include "HeaderStructs.hpp"
#include "Decompression.hpp"

int processPic(std::istream &in, const fs::path &output) {
	PicHeader header;
	in >> header;

	Image result({header.width, header.height}), currentChunk({0, 0});
	std::vector<MaskRect> maskData;

	for(size_t i = 0; i < header.chunks.size(); i++) {
		const auto &chunk = header.chunks[i];
		processChunk(currentChunk, maskData, chunk.offset, in, "chunk" + std::to_string(i), header.isSwitch);
		currentChunk.drawOnto(result, {chunk.x, chunk.y}, maskData);
	}

	result.writePNG(output);
	return 0;
}

int replacePic(std::istream &in, const fs::path &output, const fs::path &replacementFile) {
	PicHeader header;
	in >> header;

	// Games like to use 256x128 chunks so we will too
	constexpr static int CHUNK_WIDTH = 256;
	constexpr static int CHUNK_HEIGHT = 128;

	Compressor compressor;
	Image replacement = Image::readPNG(replacementFile);
	std::vector<std::pair<ChunkHeader, std::vector<uint8_t>>> data;
	std::vector<PicChunk> chunkEntries;
	Size sizeInChunks = {(replacement.size.width + CHUNK_WIDTH - 1) / CHUNK_WIDTH, (replacement.size.height + CHUNK_HEIGHT - 1) / CHUNK_HEIGHT};
	data.resize(sizeInChunks.area());
	header.chunks.resize(sizeInChunks.area());
	int pos = header.binSize();
	Image chunk;
	int headerIdx = 0;
	// Games like to use 256x128 chunks so we will too
	for (int y1 = 0; y1 < replacement.size.height; y1 += CHUNK_HEIGHT) {
		int height = std::min(replacement.size.height - y1, CHUNK_HEIGHT);
		for (int x1 = 0; x1 < replacement.size.width; x1 += CHUNK_WIDTH) {
			pos = (pos + 15) / 16 * 16;
			int width = std::min(replacement.size.width - x1, CHUNK_WIDTH);
			chunk.fastResize({width, height});
			replacement.drawOnto(chunk, {0, 0}, {x1, y1}, {width, height});
			MaskRect bounds = {0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height)};
			auto& chunkEntry = data[headerIdx];
			auto& headerEntry = header.chunks[headerIdx];
			headerIdx++;
			chunkEntry.first = compressor.encodeChunk(chunkEntry.second, chunk, bounds, {0, 0}, header.isSwitch);

			headerEntry.x = x1;
			headerEntry.y = y1;
			headerEntry.offset = pos;
			headerEntry.size = chunkEntry.first.calcAlignmentGetBinSize() + chunkEntry.second.size();
			pos += headerEntry.size;
		}
	}

	fs::ofstream outfile(output, std::ios::binary);
	header.filesize = pos;
	header.write(outfile, in);
	for (int i = 0; i < sizeInChunks.area(); i++) {
		outfile.seekp(header.chunks[i].offset, outfile.beg);
		outfile << data[i].first;
		outfile.write(reinterpret_cast<char*>(data[i].second.data()), data[i].second.size());
	}
	return 0;
}
