#include "runtime/editor/converters/null/converter_null.h"
#include "runtime/editor/dialog/dialog_message.h"
#include "runtime/editor/components_manager.h"
#include "utils/c64file.h"
#include "foundation/base/assert.h"

namespace Editor
{
	ConverterNull::ConverterNull()
	{
	}

	ConverterNull::~ConverterNull()
	{
	}

	const std::string ConverterNull::GetName() const { return "NULL converter"; }

	bool ConverterNull::CanConvert(const void* inData, unsigned int inDataSize) const
	{
		// Assert that there's is some data in the first place
		FOUNDATION_ASSERT(inData != nullptr);
		FOUNDATION_ASSERT(inDataSize > 0);

		return false;
	}


	bool ConverterNull::ConsumeKeyEvent(SDL_Keycode inKeyEvent, unsigned int inModifiers)
	{
		return false;
	}


	bool ConverterNull::Update() 
	{
		FOUNDATION_ASSERT(GetState() != State::Uninitialized);

		if (GetState() == State::Initialized)
		{
			m_State = State::Completed;

			// Show a dialog, to test the flow!
			m_ComponentsManager->StartDialog(std::make_shared<DialogMessage>("NULL Converter", "You will now be converting this file to a null thing", 80, true, [&]()
			{
			}
			));
		}

		// Return true, to indicate that the convertion 
		return true;
	}


	void ConverterNull::Setup()
	{

	}
}