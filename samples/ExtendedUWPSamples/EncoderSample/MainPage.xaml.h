/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
\**********************************************************************************/

#pragma once

#include "MainPage.g.h"

using namespace concurrency;
using namespace MSDKInterop; 

namespace EncoderSample
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

		void ReportProgress(EncoderInterop^ sender, double progress);
		void OnEncodeComplete(EncoderInterop^ sender, Windows::Foundation::AsyncStatus status);
		void MediaFailed(Platform::Object^ sender, Windows::UI::Xaml::ExceptionRoutedEventArgs^ e);

	private:
		Windows::Foundation::EventRegistrationToken regToken_for_ToggleEncoding;
		bool m_bEncoding;

		Windows::Storage::StorageFile^ m_StorageSource;
		Windows::Storage::StorageFile^ m_StorageSink;

		task<Windows::Storage::StorageFile^> ShowFileOpenPicker();
		task<Windows::Storage::StorageFile^> ShowFileSavePicker();

		void FileOpenPickerClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void FileSavePickerClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		void ToggleEncodingOn(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void DoEncoding();
		void ToggleEncodingOff(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		void ScheduleToGuiThread(std::function<void()> p);
		void HandleFilePickerException();
		void InvalidateControlsState();
		
		DecoderInterop^ m_decoder;
		EncoderInterop^  m_encoder;

		void Pivot_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
		void OnPlayTabActivated();

		bool isEncoded;

		Windows::Storage::StorageFolder^ defaultOutputFolder;
		Platform::String^ defaultOutputFilename;
		void textBoxWidth_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void textBoxHeight_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void PaintGUIToNonEncodingState();
	};
}
