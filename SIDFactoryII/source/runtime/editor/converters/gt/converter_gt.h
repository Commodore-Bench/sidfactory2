#pragma once

#include "runtime/editor/converters/converterbase.h"

namespace Editor
{
	class ComponentConsole;

	class ConverterGT : public ConverterBase
	{
	public:
		ConverterGT();
		virtual ~ConverterGT();

		const std::string GetName() const override;
		bool CanConvert(const void* inData, unsigned int inDataSize) const override;
		bool ConsumeKeyEvent(SDL_Keycode inKeyEvent, unsigned int inModifiers) override;
		bool Update() override;

	private:
		bool CanConvertInput(void* inData, unsigned int inDataSize) const;
		void Setup() override;

		// Console
		std::shared_ptr<ComponentConsole> m_Console;
	};
}