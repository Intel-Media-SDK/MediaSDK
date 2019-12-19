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

using namespace PlayerSample;
using namespace MSDKInterop;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

using namespace Windows::Media::Core;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Popups;

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

using namespace concurrency;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
}

void MainPage::Page_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	TimeSpan ts;
	ts.Duration = 500;
	timer->Interval = ts;
	timer->Tick += ref new Windows::Foundation::EventHandler<Platform::Object ^>(this, &MainPage::OnTick);
	timer->Start();
}

void MainPage::OpenLocalFile(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Windows::Storage::Pickers::FileOpenPicker^ picker = ref new Windows::Storage::Pickers::FileOpenPicker();
	picker->ViewMode = Windows::Storage::Pickers::PickerViewMode::Thumbnail;
	picker->SuggestedStartLocation = Windows::Storage::Pickers::PickerLocationId::ComputerFolder;
	picker->FileTypeFilter->Append(".h264");
	picker->FileTypeFilter->Append(".264");
	picker->FileTypeFilter->Append(".264");
	picker->FileTypeFilter->Append(".m2v");
	picker->FileTypeFilter->Append(".mpg");
	picker->FileTypeFilter->Append(".bs");
	picker->FileTypeFilter->Append(".es");
	picker->FileTypeFilter->Append(".h265");
	picker->FileTypeFilter->Append(".265");
	picker->FileTypeFilter->Append(".hevc");

	create_task(picker->PickSingleFileAsync()).then([this](Windows::Storage::StorageFile^ file)
	{
		if (file)
		{

			mediaElement->Stop();
			mediaElement->Source = nullptr;

			try
			{
				try
				{
					msdkMSS = DecoderInterop::CreatefromFile(file);
				}
				catch (...)
				{
					int i = 0;
				}

				if (msdkMSS != nullptr)
				{
					try
					{
						MediaStreamSource^ mss = msdkMSS->GetMediaStreamSource();

						if (mss)
						{
							mediaElement->SetMediaStreamSource(mss);
							Splitter->IsPaneOpen = false;
						}
						else
						{
							DisplayErrorMessage("Cannot Open Media");
						}
					}
					catch (...)
					{
						int i = 0;
					}

				}
				else
				{
					DisplayErrorMessage("Cannot open media");

				}
			}
			catch (COMException^ ex)
			{
				DisplayErrorMessage(ex->Message);
			}
		}
	});
}

void PlayerSample::MainPage::OnTick(Platform::Object ^ sender, Platform::Object ^ args)
{
}

void MainPage::MediaFailed(Platform::Object^ sender, Windows::UI::Xaml::ExceptionRoutedEventArgs^ e)
{
	DisplayErrorMessage(e->ErrorMessage);
}

void MainPage::DisplayErrorMessage(Platform::String^ message)
{
	// Display error message
	auto errorDialog = ref new MessageDialog(message);
	errorDialog->ShowAsync();
}