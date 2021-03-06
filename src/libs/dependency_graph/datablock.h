#pragma once

#include <vector>
#include <memory>

#include "data.h"
#include "metadata.h"

namespace dependency_graph {

class Node;
class Port;

/// A data storage class used by Node implementation.
/// Each data value is strongly typed, and stored as base class pointer.
class Datablock {
	public:
		Datablock(const MetadataHandle& meta);

		Datablock(const Datablock& d);
		Datablock& operator = (const Datablock& d);

		template<typename T>
		const T& get(size_t index) const;

		template<typename T>
		void set(size_t index, const T& value);

		void set(size_t index, const Port& port);

		void reset(size_t index);

		const BaseData& data(size_t index) const;
		void setData(size_t index, const BaseData& data);
		bool isNull(std::size_t index) const;

		const MetadataHandle& meta() const;

	private:
		std::vector<std::unique_ptr<BaseData>> m_data;
		MetadataHandle m_meta;
};

}
