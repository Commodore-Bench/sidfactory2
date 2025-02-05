#pragma once

#include "runtime/editor/driver/driver_info.h"
#include <vector>

namespace Editor
{
	class DataSourceTable;

	class InstrumentDataTableMapping
	{
	public:
		explicit InstrumentDataTableMapping(const DataSourceTable* inTableData, const DriverInfo::InstrumentDataPointerDescription& inTablePointerDescription);
		~InstrumentDataTableMapping();

		bool BuildFrom(unsigned int inIndex);

	private:
		unsigned int GetNextIndex(unsigned int inIndex);

		const DataSourceTable* m_TableData;
		const DriverInfo::InstrumentDataPointerDescription& m_TablePointerDescription;

		int m_HighestIndex;
		std::vector<int> m_Indices;
	};
}