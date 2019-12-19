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
#include "NumEditControl.h"
#include <string>

using namespace EncoderSample;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Documents;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

NumEditControl::NumEditControl()
{
	this->TextWrapping = Windows::UI::Xaml::TextWrapping::NoWrap;
	PlaceholderText = "Default";
}

void NumEditControl::OnLostFocus(Windows::UI::Xaml::RoutedEventArgs^ e)
{
	int val = _wtoi(this->Text->Data());
	if (val == 0 && this->Text != L"0")
	{
		this->Text = noDefault ? minValue.ToString() : L"";
	}
	else if (val < minValue)
	{
		this->Text = minValue.ToString();
	}
	else if (val > maxValue)
	{
		this->Text = maxValue.ToString();
	}

	__super::OnLostFocus(e);
}

int NumEditControl::GetValue()
{
	return _wtoi(this->Text->Data());
}