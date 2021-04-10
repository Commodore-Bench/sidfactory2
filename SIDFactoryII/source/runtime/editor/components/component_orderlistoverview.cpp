#include "runtime/editor/components/component_orderlistoverview.h"
#include "runtime/editor/components/utils/text_editing_data_source_table_text.h"
#include "runtime/editor/cursor_control.h"
#include "runtime/editor/display_state.h"
#include "runtime/editor/datasources/datasource_orderlist.h"
#include "runtime/editor/datasources/datasource_sequence.h"
#include "runtime/editor/datasources/datasource_table_text.h"
#include "foundation/graphics/textfield.h"
#include "foundation/input/keyboard.h"
#include "foundation/input/mouse.h"
#include "foundation/base/assert.h"
#include "utils/usercolors.h"

using namespace Foundation;
using namespace Utility;

namespace Editor
{
	const int ComponentOrderListOverview::m_MarginWidth = 1;
	const int ComponentOrderListOverview::m_TextWidth = 19;

	int ComponentOrderListOverview::GetWidthFromChannelCount(int inChannelCount)
	{
		// Example, 3 rows: " xxxx: 01 02 03 abcdef0123456789 "
		return m_MarginWidth + 6 + 3 * inChannelCount + m_TextWidth;
	}


	ComponentOrderListOverview::ComponentOrderListOverview(
		int inID,
		int inGroupID, 
		Undo* inUndo,
		TextField* inTextField,
		std::shared_ptr<DataSourceTableText> inDataSourceTableText,
		const std::vector<std::shared_ptr<DataSourceOrderList>>& inOrderLists,
		const std::vector<std::shared_ptr<DataSourceSequence>>& inSequenceList,
		int inX,
		int inY,
		int inHeight,
		std::function<void(int, bool)> inSetTrackEventPosFunction
	)
		: ComponentBase(inID, inGroupID, inUndo, inTextField, inX, inY, ComponentOrderListOverview::GetWidthFromChannelCount(static_cast<int>(inOrderLists.size())), inHeight)
		, m_TableText(inDataSourceTableText)
		, m_OrderLists(inOrderLists)
		, m_SequenceList(inSequenceList)
		, m_PlaybackEventPosition(-1)
		, m_CursorPosition(0)
		, m_MaxCursorPosition(0)
		, m_TopPosition(0)
		, m_SetTrackEventPosFunction(inSetTrackEventPosFunction)
	{
		m_TextEditingDataSourceTableText = std::make_unique<TextEditingDataSourceTableText>(
			inUndo, 
			inID,
			inGroupID,
			inDataSourceTableText, 
			m_TextWidth);
	}


	ComponentOrderListOverview::~ComponentOrderListOverview()
	{

	}


	void ComponentOrderListOverview::SetHasControl(GetControlType inGetControlType, CursorControl& inCursorControl)
	{
		// Set has control, but do not enaBle the cursor for this component, because it doesn't need it yet!
		m_HasControl = true;
		m_RequireRefresh = true;
	}



	bool ComponentOrderListOverview::ConsumeInput(const Keyboard& inKeyboard, CursorControl& inCursorControl, ComponentsManager& inComponentsManager)
	{
		bool consume = false;

		if (IsEditingText())
			consume = m_TextEditingDataSourceTableText->ConsumeKeyboardInput(inKeyboard, inCursorControl);
		else
		{
			const bool isNoModifierDown = inKeyboard.IsModifierEmpty();
			const bool isOnlyShiftDown = inKeyboard.IsModifierDownExclusive(Keyboard::Shift);

			for (const auto& key_event : inKeyboard.GetKeyEventList())
			{
				switch (key_event)
				{
				case SDLK_DOWN:
					if (inKeyboard.IsModifierEmpty() && DoCursorDown(1))
					{
						m_RequireRefresh = true;
						consume = true;
					}
					break;
				case SDLK_UP:
					if (inKeyboard.IsModifierEmpty() && DoCursorUp(1))
					{
						m_RequireRefresh = true;
						consume = true;
					}
					break;
				case SDLK_PAGEDOWN:
					if (DoCursorDown(20))
					{
						m_RequireRefresh = true;
						consume = true;
					}
					break;
				case SDLK_PAGEUP:
					if (DoCursorUp(20))
					{
						m_RequireRefresh = true;
						consume = true;
					}
					break;
				case SDLK_HOME:
					if (DoHome())
					{
						m_RequireRefresh = true;
						consume = true;
					}
					break;
				case SDLK_END:
					if (DoEnd())
					{
						m_RequireRefresh = true;
						consume = true;
					}
					break;
				case SDLK_RETURN:
					if (isOnlyShiftDown)
					{
						if (m_CursorPosition >= 0 && m_CursorPosition < static_cast<int>(m_Overview.size()) && m_SetTrackEventPosFunction)
						{
							m_SetTrackEventPosFunction(m_Overview[m_CursorPosition].m_EventPos, inKeyboard.IsModifierDownExclusive(Keyboard::Control));
							consume = true;
						}
					}
					else if (isNoModifierDown)
					{
						DoStartEditText(inCursorControl);
						m_RequireRefresh = true;
						consume = true;
					}
					break;
				}
			}
		}

		if (m_TextEditingDataSourceTableText->RequireRefresh())
		{
			m_RequireRefresh = true;
			m_TextEditingDataSourceTableText->ResetRequireRefresh();
		}

		return consume;
	}


	bool ComponentOrderListOverview::ConsumeInput(const Foundation::Mouse& inMouse, bool inModifierKeyMask, CursorControl& inCursorControl, ComponentsManager& inComponentsManager)
	{
		bool consume = false;

		if (ContainsPosition(inMouse.GetPosition()))
		{
			if (inMouse.IsButtonDoublePressed(Mouse::Left))
			{
				Point cell_position = GetLocalCellPosition(inMouse.GetPosition());
				m_CursorPosition = cell_position.m_Y + m_TopPosition;

				if (m_CursorPosition >= 0 && m_CursorPosition < static_cast<int>(m_Overview.size()) && m_SetTrackEventPosFunction)
					m_SetTrackEventPosFunction(m_Overview[m_CursorPosition].m_EventPos, true);

				m_RequireRefresh = true;

				consume = true;
			}
			else if (inMouse.IsButtonPressed(Mouse::Left))
			{
				Point local_cell_position = GetLocalCellPosition(inMouse.GetPosition());
				m_CursorPosition = local_cell_position.m_Y + m_TopPosition;

				if (m_CursorPosition >= 0 && m_CursorPosition < static_cast<int>(m_Overview.size()) && m_SetTrackEventPosFunction)
					m_SetTrackEventPosFunction(m_Overview[m_CursorPosition].m_EventPos, false);

				m_RequireRefresh = true;

				const int text_x = m_Dimensions.m_Width - m_TextWidth;
				const int text_x_right = m_Dimensions.m_Width;

				if (local_cell_position.m_X >= text_x && local_cell_position.m_X <= text_x_right)
				{
					if (IsEditingText() && m_CursorPosition != m_TextEditingDataSourceTableText->GetTextLineIndex())
						DoStopEditText(inCursorControl, false);
					if (!IsEditingText())
						DoStartEditText(inCursorControl);

					const int cursor_pos = local_cell_position.m_X - text_x;

					m_TextEditingDataSourceTableText->TrySetCursorPosition(cursor_pos);
				}
				else
				{
					if (IsEditingText())
						DoStopEditText(inCursorControl, false);
				}

				consume = true;
			}
		}
		
		DoMouseWheel(inMouse);

		return consume;
	}


	void ComponentOrderListOverview::ConsumeNonExclusiveInput(const Mouse& inMouse)
	{
		if (!m_HasControl)
		{
			DoMouseWheel(inMouse);
		}
	}



	void ComponentOrderListOverview::Refresh(const DisplayState& inDisplayState)
	{
		if (m_RequireRefresh)
		{
			RebuildOverview();

			m_TextField->Clear(m_Rect);
			
			Rect ListRect = m_Rect;
			ListRect.m_Dimensions.m_Width -= m_TextWidth;

			Rect TextRect = m_Rect;
			TextRect.m_Position.m_X += ListRect.m_Dimensions.m_Width;
			TextRect.m_Dimensions.m_Width -= ListRect.m_Dimensions.m_Width;
			
			m_TextField->ColorAreaBackground(ToColor(UserColor::SongListBackground), ListRect);
			m_TextField->ColorAreaBackground(ToColor(UserColor::SongListBackgroundText), TextRect);

			const bool is_uppercase = inDisplayState.IsHexUppercase();
			const int cursor_position = m_CursorPosition - m_TopPosition;

			if (cursor_position >= 0 && cursor_position < m_Dimensions.m_Height)
				m_TextField->ColorAreaBackground(ToColor(m_HasControl ? UserColor::SongListCursorFocus : UserColor::SongListCursorDefault), { m_Position + Point(0, cursor_position), { m_Dimensions.m_Width, 1 } });

			int local_y = 0;
			int overview_list_size = static_cast<int>(m_Overview.size());

			Color event_pos_values = ToColor(UserColor::SongListEventPos);

			for (int i=m_TopPosition; i < overview_list_size && local_y < m_Dimensions.m_Height; ++i)
			{
				OverviewEntry& entry = m_Overview[i];

				int x = m_Position.m_X + 1;
				int y = local_y + m_Position.m_Y;

				m_TextField->PrintHexValue(x, y, event_pos_values, is_uppercase, static_cast<unsigned short>(entry.m_EventPos));

				const int next_event_pos = i < overview_list_size - 1 ? m_Overview[i + 1].m_EventPos : 0x7fffffff;

				if(m_PlaybackEventPosition >= 0 && m_PlaybackEventPosition >= entry.m_EventPos && m_PlaybackEventPosition < next_event_pos)
					m_TextField->ColorAreaBackground(ToColor(UserColor::SongListPlaybackMarker), Rect({ { x, y}, { 4, 1 } }));

				m_TextField->Print(x + 4, y, event_pos_values, ":");

				x += 6;

				for (int sequence_index : entry.m_SequenceIndices)
				{
					if (sequence_index >= 0)
					{
						const Color color = sequence_index < 0x100 ? ToColor(UserColor::SongListValues) : ToColor(UserColor::SongListLoopMarker);
						m_TextField->PrintHexValue(x, y, color, is_uppercase, static_cast<unsigned char>(sequence_index & 0x0ff));
					}

					x += 3;
				}

				int text_x = m_Position.m_X + m_MarginWidth + 5 + 3 * m_OrderLists.size() + 1;

				if(i < m_TableText->GetSize())
					m_TextField->Print(text_x, y, ToColor(UserColor::ConsoleText), (*m_TableText)[i], m_TextWidth);

				++local_y;
			}

			m_RequireRefresh = false;
		}
	}


	bool ComponentOrderListOverview::HasDataChange() const
	{
		if (m_TextEditingDataSourceTableText->HasDataChange())
			return true;

		return ComponentBase::HasDataChange();
	}


	void ComponentOrderListOverview::HandleDataChange()
	{
		if (m_TextEditingDataSourceTableText->HasDataChange())
			m_TextEditingDataSourceTableText->ApplyDataChange();

		m_HasDataChange = false;
	}

	
	void ComponentOrderListOverview::PullDataFromSource(const bool inFromUndo)
	{
		m_TableText->PullDataFromSource();
	}


	bool ComponentOrderListOverview::IsNoteInputSilenced() const
	{
		return IsEditingText();
	}


	void ComponentOrderListOverview::ExecuteInsertDeleteRule(const DriverInfo::TableInsertDeleteRule& inRule, int inSourceTableID, int inIndexPre, int inIndexPost)
	{

	}


	void ComponentOrderListOverview::ExecuteAction(int inActionInput)
	{

	}


	void ComponentOrderListOverview::TellPlaybackEventPosition(int inPlaybackEventPosition)
	{
		if (m_PlaybackEventPosition != inPlaybackEventPosition)
		{
			m_PlaybackEventPosition = inPlaybackEventPosition;
			m_RequireRefresh = true;
		}
	}


	void ComponentOrderListOverview::DoMouseWheel(const Foundation::Mouse& inMouse)
	{
		Point scroll_wheel = inMouse.GetWheelDelta();

		if (scroll_wheel.m_Y != 0)
		{
			Point screen_position = inMouse.GetPosition();
			if (ContainsPosition(screen_position))
			{
				const int visible_height = m_Dimensions.m_Height - 1;
				const int max_top_pos = m_MaxCursorPosition < visible_height ? 0 : (m_MaxCursorPosition - visible_height);

				int top_pos = m_TopPosition;
				int change = scroll_wheel.m_Y;

				while (change > 0)
				{
					--top_pos;
					--change;
				}
				while (change < 0)
				{
					++top_pos;
					++change;
				}

				top_pos = top_pos < 0 ? 0 : top_pos > max_top_pos ? max_top_pos : top_pos;

				if (top_pos != m_TopPosition)
				{
					m_TopPosition = top_pos;

					if (m_CursorPosition < m_TopPosition)
						m_CursorPosition = m_TopPosition;
					if (m_CursorPosition > m_TopPosition + visible_height)
						m_CursorPosition = m_TopPosition + visible_height;

					m_RequireRefresh = true;
				}
			}
		}
	}


	bool ComponentOrderListOverview::DoCursorUp(unsigned int inSteps)
	{
		if (m_CursorPosition > 0)
		{
			m_CursorPosition -= inSteps;

			if (m_CursorPosition < 0)
				m_CursorPosition = 0;

			if (m_CursorPosition < m_TopPosition)
				m_TopPosition = m_CursorPosition;

			return true;
		}

		return false;
	}


	bool ComponentOrderListOverview::DoCursorDown(unsigned int inSteps)
	{
		if (m_CursorPosition < m_MaxCursorPosition)
		{
			m_CursorPosition += inSteps;

			if (m_CursorPosition > m_MaxCursorPosition)
				m_CursorPosition = m_MaxCursorPosition;

			if (m_CursorPosition >= m_TopPosition + m_Dimensions.m_Height)
				m_TopPosition = m_CursorPosition - (m_Dimensions.m_Height - 1);

			return true;
		}

		return false;
	}


	bool ComponentOrderListOverview::DoHome()
	{
		if (m_CursorPosition > 0)
		{
			if (m_CursorPosition == m_TopPosition)
			{
				m_CursorPosition = 0;
				m_TopPosition = 0;
			}
			else
				m_CursorPosition = m_TopPosition;

			return true;
		}

		return false;
	}

	bool ComponentOrderListOverview::DoEnd()
	{
		if (m_CursorPosition < m_MaxCursorPosition)
		{
			int max_visible = m_TopPosition + m_Dimensions.m_Height - 1;

			if (max_visible <= m_MaxCursorPosition && m_CursorPosition < max_visible)
			{
				m_CursorPosition = max_visible;
			}
			else
			{
				m_CursorPosition = m_MaxCursorPosition;

				if (m_CursorPosition >= m_TopPosition + m_Dimensions.m_Height)
					m_TopPosition = m_CursorPosition - (m_Dimensions.m_Height - 1);
			}

			return true;
		}

		return false;
	}


	bool ComponentOrderListOverview::IsEditingText() const
	{
		return m_TextEditingDataSourceTableText->IsEditing();
	}


	void ComponentOrderListOverview::DoStartEditText(CursorControl& inCursorControl)
	{
		auto GetEditingRowIndex = [&]() -> unsigned int
		{
			return static_cast<unsigned int>(m_CursorPosition);
		};

		if (!IsEditingText())
		{
			m_TextEditingDataSourceTableText->StartEditing(GetEditingRowIndex(), GetEditingTextScreenPosition());
			inCursorControl.SetEnabled(true);
		}
	}


	void ComponentOrderListOverview::DoStopEditText(CursorControl& inCursorControl, bool inCancel)
	{
		m_TextEditingDataSourceTableText->StopEditing(inCancel, inCursorControl);
		inCursorControl.SetEnabled(false);
	}


	Foundation::Point ComponentOrderListOverview::GetEditingTextScreenPosition() const
	{
		return Foundation::Point(m_Position.m_X + m_Rect.m_Dimensions.m_Width - m_TextWidth, m_Position.m_Y + m_CursorPosition - m_TopPosition);
	}

	void ComponentOrderListOverview::RebuildOverview()
	{
		m_Overview.clear();

		std::vector<int> orderlist_indices;
		std::vector<int> orderlist_event_pos;

		const int channel_count = static_cast<int>(m_OrderLists.size());

		for (int i = 0; i < channel_count; ++i)
		{
			orderlist_indices.push_back(0);
			orderlist_event_pos.push_back(0);
		}

		int event_pos = 0;

		while (event_pos < 0x7fffffff)
		{
			OverviewEntry entry;

			entry.m_EventPos = event_pos;

			// Construct the next entry
			for (int i = 0; i < channel_count; ++i)
			{
				const int loop_index = static_cast<int>(m_OrderLists[i]->GetLoopIndex());
				if (orderlist_event_pos[i] == event_pos)
				{
					const int orderlist_index = orderlist_indices[i];
					const auto& order_list_entry = (*m_OrderLists[i])[orderlist_index];
					bool is_end = order_list_entry.m_Transposition >= 0xfe;

					if (!is_end)
					{
						if(orderlist_index == loop_index)
							entry.m_SequenceIndices.push_back(order_list_entry.m_SequenceIndex | 0x100);
						else
							entry.m_SequenceIndices.push_back(order_list_entry.m_SequenceIndex);

						orderlist_event_pos[i] += m_SequenceList[order_list_entry.m_SequenceIndex]->GetLength();
						orderlist_indices[i]++;
					}
					else
					{
						entry.m_SequenceIndices.push_back(-1);
						orderlist_event_pos[i] = -1;
					}
				}
				else 
					entry.m_SequenceIndices.push_back(-1);
			}

			m_Overview.push_back(entry);

			int event_pos_forward = [&]()
			{
				int closest_event_pos = 0x7fffffff;

				for (const int& next_event_pos : orderlist_event_pos)
				{
					if (next_event_pos >= 0)
					{
						if (next_event_pos < closest_event_pos)
							closest_event_pos = next_event_pos;
					}
				}

				return closest_event_pos;
			}();

			event_pos = event_pos_forward;
		}

		m_MaxCursorPosition = static_cast<int>(m_Overview.size()) - 1;

		if (m_CursorPosition > m_MaxCursorPosition)
			m_CursorPosition = m_MaxCursorPosition;
	}
}
