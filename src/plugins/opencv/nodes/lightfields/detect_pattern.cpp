#include <possumwood_sdk/node_implementation.h>

#include <opencv2/opencv.hpp>

#include <actions/traits.h>

#include <lightfields/lenslet_graph.h>
#include <lightfields/pattern.h>

#include "../../lightfield_pattern.h"

#include "frame.h"

namespace {

struct Vec2Compare {
	bool operator() (const cv::Vec2f& v1, const cv::Vec2f& v2) const {
		if(v1[0] != v2[0])
			return v1[0] < v2[0];
		return v1[1] < v2[1];
	}
};

struct Vec4Compare {
	bool operator() (const cv::Vec4f& v1, const cv::Vec4f& v2) const {
		for(int a=0;a<3;++a)
			if(v1[a] != v2[a])
				return v1[a] < v2[a];
		return v1[3] < v2[3];
	}
};

dependency_graph::InAttr<possumwood::opencv::Frame> a_in;
dependency_graph::InAttr<unsigned> a_border;
dependency_graph::OutAttr<possumwood::opencv::Frame> a_out;
dependency_graph::OutAttr<lightfields::Pattern> a_pattern;

dependency_graph::State compute(dependency_graph::Values& data) {
	const cv::Mat& input = *data.get(a_in);
	if(input.type() != CV_8UC1)
		throw std::runtime_error("Only CV_8UC1 type allowed on input");

	const std::size_t border = data.get(a_border);
	if(border < 1)
		throw std::runtime_error("Border value needs to be non-zero");
	if(std::size_t(input.rows) < 2*border+1 || std::size_t(input.cols) < 2*border+1)
		throw std::runtime_error("Border value too large for the size of the input image");

	// make a lenslet data structure
	lightfields::LensletGraph lenslets(cv::Vec2i(input.cols, input.rows), border);
	// feed all the local minima into it, except the ones too close to the border
	for(std::size_t y=1; y<std::size_t(input.rows) - 1; ++y)
		for(std::size_t x=1; x<std::size_t(input.cols) - 1; ++x) {
			const unsigned char current = input.at<unsigned char>(y, x);
			bool isMaximum = true;
			for(std::size_t a=0;a<9;++a)
				if(a != 4 && input.at<unsigned char>(y + a/3 - 1, x + a%3 - 1) >= current)
					isMaximum = false;

			if(isMaximum)
				lenslets.addLenslet(cv::Vec2i(x, y));
		}

	// debug drawing
	cv::Mat mat(input.size(), CV_8UC1, cv::Scalar(0));
	lenslets.drawCenters(mat);

	// delaunay drawing
	auto fitted = lenslets.fit();
	lenslets.drawFit(mat);

	data.set(a_out, possumwood::opencv::Frame(mat));

	// pattern
	Imath::M33d fittedMatrix(
		fitted(0, 0), fitted(0, 1), fitted(0, 2),
		fitted(1, 0), fitted(1, 1), fitted(1, 2),
		fitted(2, 0), fitted(2, 1), fitted(2, 2)
	);

	data.set(a_pattern, lightfields::Pattern(lenslets.lensPitch(), fittedMatrix, Imath::V2i(input.cols, input.rows)));

	return dependency_graph::State();
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(a_in, "in_frame", possumwood::opencv::Frame(), possumwood::AttrFlags::kVertical);
	meta.addAttribute(a_border, "border", 20u);
	meta.addAttribute(a_out, "out_frame", possumwood::opencv::Frame(), possumwood::AttrFlags::kVertical);
	meta.addAttribute(a_pattern, "pattern", lightfields::Pattern(), possumwood::AttrFlags::kVertical);

	meta.addInfluence(a_in, a_out);
	meta.addInfluence(a_border, a_out);
	meta.addInfluence(a_in, a_pattern);
	meta.addInfluence(a_border, a_pattern);

	meta.setCompute(compute);
}

possumwood::NodeImplementation s_impl("opencv/lightfields/detect_pattern", init);

}
