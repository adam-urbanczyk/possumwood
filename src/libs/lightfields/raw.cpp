#include "raw.h"

#include <sstream>
#include <cassert>

#include <json/reader.h>

#include "block.h"

namespace lightfields {

namespace {

template<typename T>
std::string str(const T& val) {
	return std::to_string(val);
}

template<>
std::string str<std::string>(const std::string& val) {
	return val;
}

template<typename T>
void checkThrow(const T& value, const T& expected, const std::string& error) {
	if(value != expected)
		throw std::runtime_error("Expected " + error + " " + str(expected) + ", got " + str(value) + "!");
}

}

struct Raw::Pimpl {
	Json::Value header, metadata, privateMetadata;
	std::vector<char> data;
};

Raw::Raw() : m_pimpl(new Pimpl()) {
}

Raw::~Raw() {
}

const Json::Value& Raw::header() const {
	return m_pimpl->header;
}

const Json::Value& Raw::metadata() const {
	return m_pimpl->metadata;
}

const Json::Value& Raw::privateMetadata() const {
	return m_pimpl->privateMetadata;
}

const std::vector<char> Raw::image() const {
	return m_pimpl->data;
}

std::istream& operator >> (std::istream& in, Raw& data) {
	std::unique_ptr<Raw::Pimpl> impl(new Raw::Pimpl());

	std::string metadataRef, privateMetadataRef, imageRef;

	lightfields::Block block;

	// skip the initial block
	in >> block;
	checkThrow(block.id, 'P', "Initial P block of a lytro raw file not found.");

	while(block.id != '\0') {
		in >> block;

		if(block.id == 'M') {
			std::stringstream ss(block.data.data());
			ss >> impl->header;

			if(impl->header["frames"].size() != 1)
				throw std::runtime_error("Only single-frame raw images supported at the moment.");

			metadataRef = impl->header["frames"][0]["frame"]["metadataRef"].asString();
			privateMetadataRef = impl->header["frames"][0]["frame"]["privateMetadataRef"].asString();
			imageRef = impl->header["frames"][0]["frame"]["imageRef"].asString();
		}

		else if(block.name == metadataRef) {
			std::stringstream ss(block.data.data());
			ss >> impl->metadata;

			checkThrow(impl->metadata["image"]["width"].isInt(), true, "width");
			checkThrow(impl->metadata["image"]["height"].isInt(), true, "height");

			checkThrow(impl->metadata["image"]["orientation"].asInt(), 1, "orientation");
			checkThrow(impl->metadata["image"]["representation"].asString(), std::string("rawPacked"), "representation");
			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["rightShift"].asInt(), 0, "rightShift");

			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["black"].size(), 4u, "black size");
			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["white"].size(), 4u, "white size");

			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["black"]["b"].isInt(), true, "[black][b]");
			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["black"]["gb"].isInt(), true, "[black][gb]");
			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["black"]["gr"].isInt(), true, "[black][gr]");
			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["black"]["r"].isInt(), true, "[black][r]");

			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["white"]["b"].isInt(), true, "[white][b]");
			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["white"]["gb"].isInt(), true, "[white][gb]");
			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["white"]["gr"].isInt(), true, "[white][gr]");
			checkThrow(impl->metadata["image"]["rawDetails"]["pixelFormat"]["white"]["r"].isInt(), true, "[white][r]");

			checkThrow(impl->metadata["image"]["rawDetails"]["pixelPacking"]["endianness"].asString(), std::string("big"), "endianness");
			checkThrow(impl->metadata["image"]["rawDetails"]["pixelPacking"]["bitsPerPixel"].asInt(), 12, "bitsPerPixel");

			checkThrow(impl->metadata["image"]["rawDetails"]["mosaic"]["tile"].asString(), std::string("r,gr:gb,b"), "mosaic/tile");
			checkThrow(impl->metadata["image"]["rawDetails"]["mosaic"]["upperLeftPixel"].asString(), std::string("b"), "mosaic/upperLeftPixel");
		}

		else if(block.name == privateMetadataRef) {
			std::stringstream ss(block.data.data());
			ss >> impl->privateMetadata;
		}

		// just copy the image data
		else if(block.name == imageRef) {
			assert(!block.data.empty());
			impl->data = std::move(block.data);
		}

		// temporary - printouts of headers
		// if((block.data.size() > 1) && (block.data[0] == '{'))
		// 	std::cout << block.data.data() << std::endl;
	}

	data.m_pimpl = std::shared_ptr<const Raw::Pimpl>(impl.release());

	return in;
}

}
