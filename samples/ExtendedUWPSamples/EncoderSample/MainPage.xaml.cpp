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

#include "pch.h"
#include "MainPage.xaml.h"
#include "EncoderSettingsControl.xaml.h"

using namespace EncoderSample;

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::BulkAccess;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Popups;
using namespace Windows::Media::Core;

using namespace concurrency;
using namespace MSDKInterop;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::DataTransfer;

using namespace std;
// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
	regToken_for_ToggleEncoding = buttonRunEncoding->Click +=
		ref new RoutedEventHandler(this, &MainPage::ToggleEncodingOn);
	isEncoded = false;

	defaultOutputFolder = KnownFolders::VideosLibrary;
	defaultOutputFilename = L"output.h264";

	InvalidateControlsState();
}

void MainPage::ReportProgress(EncoderInterop^ sender, double progress)
{
	progressBar->Value = progress;
	return; 
}

void MainPage::OnEncodeComplete(EncoderInterop^ sender, AsyncStatus status)
{
	if (status == AsyncStatus::Completed)
	{
		isEncoded = true;
	}
	else if (status == AsyncStatus::Canceled)
	{
	}
	else if (status == AsyncStatus::Error)
	{
		ScheduleToGuiThread([this]()
		{
			auto errorDialog = ref new MessageDialog("Error occured during encoding");
			errorDialog->ShowAsync();
		});
	}

	PaintGUIToNonEncodingState();
}

void MainPage::PaintGUIToNonEncodingState()
{
	ScheduleToGuiThread([this]()
	{
		buttonRunEncoding->Click -= regToken_for_ToggleEncoding;
		regToken_for_ToggleEncoding = buttonRunEncoding->Click +=
			ref new RoutedEventHandler(this, &MainPage::ToggleEncodingOn);
		
		buttonSelectInput->IsEnabled = true;
		buttonSelectOutput->IsEnabled = true;

		buttonRunEncoding->Content = "Encode";
		buttonRunEncoding->IsEnabled = true;
		progressBar->Value = 0;

		m_bEncoding = false;
	});
}

void MainPage::MediaFailed(Platform::Object ^ sender, Windows::UI::Xaml::ExceptionRoutedEventArgs ^ e)
{
	// Display error message
	auto errorDialog = ref new MessageDialog(e->ErrorMessage);
	errorDialog->ShowAsync();
}

task<StorageFile^> EncoderSample::MainPage::ShowFileOpenPicker()
{
	auto fileOpenPicker = ref new FileOpenPicker();
	fileOpenPicker->ViewMode = PickerViewMode::List;
	fileOpenPicker->FileTypeFilter->Clear();
	fileOpenPicker->FileTypeFilter->Append(".yuv");
	fileOpenPicker->SuggestedStartLocation = PickerLocationId::ComputerFolder;

	return create_task(fileOpenPicker->PickSingleFileAsync());
}


task<Windows::Storage::StorageFile^> EncoderSample::MainPage::ShowFileSavePicker()
{
	auto fileSavePicker = ref new FileSavePicker();

	auto extensions = ref new Platform::Collections::Vector<String^>(1, ".h264");
	fileSavePicker->FileTypeChoices->Insert("AVC/h.264 video ES", extensions);

	extensions = ref new Platform::Collections::Vector<String^>(1, ".h265");
	fileSavePicker->FileTypeChoices->Insert("HEVC/h.265 video ES", extensions);

	if (EncoderInterop::IsMP4Supported())
	{
		extensions = ref new Platform::Collections::Vector<String^>(1, ".mp4");
		fileSavePicker->FileTypeChoices->Insert("MP4 Container (h.264)", extensions);
	}

	fileSavePicker->SuggestedFileName = m_StorageSource ? m_StorageSource->Name : defaultOutputFilename;

	return create_task(fileSavePicker->PickSaveFileAsync());
}

void MainPage::FileOpenPickerClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
	ShowFileOpenPicker()
		.then([this](StorageFile^ storageSource) 
	{
		if (!storageSource)
		{
			cancel_current_task();
		}

		m_StorageSource = storageSource;
		InvalidateControlsState();
	});
}

void MainPage::FileSavePickerClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
	ShowFileSavePicker()
		.then([this](StorageFile^ storageSink) 
	{
		if (!storageSink)
		{
			cancel_current_task();
		}

		m_StorageSink = storageSink;

		//--- Changing Encoder type - some settings are not available for particular codec type
		auto codecType = CodecTypes::AVC;
		if (m_StorageSink && m_StorageSink->FileType == ".h265")
		{
			codecType = CodecTypes::HEVC;
		}
		EncodingSettings->SetCodecType(codecType);

		InvalidateControlsState();
	});

}

inline void MainPage::ScheduleToGuiThread(std::function<void()> p) 
{
	Dispatcher->RunAsync(
		Windows::UI::Core::CoreDispatcherPriority::Normal,
		ref new Windows::UI::Core::DispatchedHandler(p));
}

void MainPage::HandleFilePickerException()
{
	try {
		throw;
	}
	catch (COMException^ ex1) 
	{
	}
	catch (Platform::Exception^ ex2) 
	{
	}
	catch (...) 
	{
	}
}

void MainPage::InvalidateControlsState()
{
	ScheduleToGuiThread([=]() 
	{
		textBoxSourceFileName->Text = (m_StorageSource != nullptr) ? m_StorageSource->Path : "";
		textBoxDestinationFileName->Text = (m_StorageSink != nullptr) ? m_StorageSink->Path : defaultOutputFolder->Name+L"\\"+ defaultOutputFilename;
		buttonRunEncoding->IsEnabled = !textBoxSourceFileName->Text->IsEmpty() && !textBoxWidth->Text->IsEmpty() && !textBoxHeight->Text->IsEmpty();
	});
}

void MainPage::ToggleEncodingOn(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
	buttonSelectInput->IsEnabled = false;
	buttonSelectOutput->IsEnabled = false;

	if (!m_StorageSink)
	{
		create_task(defaultOutputFolder->CreateFileAsync(defaultOutputFilename, CreationCollisionOption::ReplaceExisting)).then([this](Concurrency::task<Windows::Storage::StorageFile ^> tsk)
		{
			m_StorageSink = tsk.get();
			DoEncoding();
		});
	}
	else
	{
		DoEncoding();
	}

}

void MainPage::ToggleEncodingOff(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
	//--- Just Stop Encoder, GUI should be updated by completed event
	buttonRunEncoding->IsEnabled = false;
	m_encoder->Stop();
}

void MainPage::DoEncoding()
{
	isEncoded = false;
	progressBar->Value = 0;

	m_encoder = EncoderInterop::Create(m_StorageSource,m_StorageSink);
	if (m_encoder != nullptr)
	{
		buttonRunEncoding->Click -= regToken_for_ToggleEncoding;
		regToken_for_ToggleEncoding = buttonRunEncoding->Click +=
			ref new RoutedEventHandler(this, &MainPage::ToggleEncodingOff);

		buttonRunEncoding->Content = "Stop";
		m_bEncoding = true;

		m_encoder->encodeProgress += ref new EncodeProgressHandler(this, &MainPage::ReportProgress);
		m_encoder->encodeComplete += ref new EncodeCompletedHandler(this, &MainPage::OnEncodeComplete);
		
		//--- Configuring encoder -----------------------------------------
		m_encoder->Width = _wtoi(textBoxWidth->Text->Begin());
		m_encoder->Height = _wtoi(textBoxHeight->Text->Begin());
		m_encoder->Framerate = _wtof(safe_cast<TextBlock^>(cmbFramerate->SelectedItem)->Text->Data());

		auto FccValue = safe_cast<TextBlock^>(cmbFourCC->SelectedItem)->Text;
		if (FccValue == "NV12")
		{
			m_encoder->SrcFourCC = MSDKInterop::SupportedFOURCC ::NV12;
		}
		else if (FccValue == "RGB4")
		{
			m_encoder->SrcFourCC = MSDKInterop::SupportedFOURCC::RGB4;
		}
		else if (FccValue == "P010")
		{
			m_encoder->SrcFourCC = MSDKInterop::SupportedFOURCC::P010;
		}
		else
		{
			m_encoder->SrcFourCC = MSDKInterop::SupportedFOURCC::I420;
		}

		EncodingSettings->ApplySettings(m_encoder);

		//--- Starting encoding ----------------------------------------------
		if (!m_encoder->StartEncoding())
		{
			auto errorDialog = ref new MessageDialog("Cannot start encoding ("+m_encoder->GetLastError()+")");
			errorDialog->ShowAsync();		
			PaintGUIToNonEncodingState();
		}
	}
	else
	{
		auto errorDialog = ref new MessageDialog("Cannot start encoding (cannot create encoder interop object)");
		errorDialog->ShowAsync();
	}
}

void MainPage::Pivot_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	PivotItem^ item = reinterpret_cast<PivotItem^>(reinterpret_cast<Pivot^>(sender)->SelectedItem);
	if (item->Name == L"playTab")
	{
		OnPlayTabActivated();
	}
}

void MainPage::OnPlayTabActivated()
{
	if (m_StorageSink  && (m_StorageSink->FileType == ".mp4" || m_StorageSink->FileType == ".MP4"))
	{
		lblNoStream->Text = "MP4 files playout is not supported";
		lblNoStream->Visibility = Windows::UI::Xaml::Visibility::Visible;
		mediaElement->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	else if (m_StorageSink && isEncoded)
	{
		lblNoStream->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		mediaElement->Visibility = Windows::UI::Xaml::Visibility::Visible;
		mediaElement->AreTransportControlsEnabled = true;

		m_decoder = DecoderInterop::CreatefromFile(m_StorageSink);
		if (m_decoder != nullptr)
		{
			MediaStreamSource^ mss = m_decoder->GetMediaStreamSource();


			if (mss)
			{
				mediaElement->SetMediaStreamSource(mss);
			}
			else
			{
				auto errorDialog = ref new MessageDialog("Cannot Open Media");
				errorDialog->ShowAsync();
			}
		}
		else
		{
			auto errorDialog = ref new MessageDialog("Cannot Open Media");
			errorDialog->ShowAsync();
		}
	}
	else
	{
		lblNoStream->Text = "Stream hasn't been encoded yet";
		lblNoStream->Visibility = Windows::UI::Xaml::Visibility::Visible;
		mediaElement->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
}


void EncoderSample::MainPage::textBoxWidth_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	InvalidateControlsState();
}


void EncoderSample::MainPage::textBoxHeight_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	InvalidateControlsState();
}
