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
#include "EncoderSettingsControl.xaml.h"

using namespace EncoderSample;
using namespace MSDKInterop;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

EncoderSettingsControl::EncoderSettingsControl()
{
	InitializeComponent();
}

void EncoderSettingsControl::ApplySettings(EncoderInterop ^ encoder)
{
	encoder->Codec = codecType;
	encoder->TargetUsage = cmbTU->SelectedIndex == 0 ? 0 : cmbTU->SelectedIndex == 1 ? 4 : 7;
	encoder->BRC = (BRCTypes)_wtoi(reinterpret_cast<ComboBoxItem^>(cmbBRC->SelectedItem)->Tag->ToString()->Data());
	encoder->MaxKbps = txtMaxKbps->GetValue();
	encoder->Quality = txtQuality->GetValue();
	encoder->QPI = txtQPI->GetValue();
	encoder->QPP = txtQPP->GetValue();
	encoder->QPB = txtQPB->GetValue();
	encoder->LAD = txtLAD->GetValue();
	encoder->Bitrate = txtBitrate->GetValue();
	encoder->LowDelayBRC = chkLowDelayBRC->IsChecked->Value;
	encoder->MaxFrameSize = txtMaxFrameSize->GetValue();
	encoder->GOPRefDist = txtGOPRefDist->GetValue();
	encoder->WinBRCSize = txtWinBRCSize->GetValue();
	encoder->WinBRCMaxAvgKbps = txtWinBRCMaxAvgKbps->GetValue();
	encoder->NumSlice = txtNumSlice->GetValue();
	encoder->MaxSliceSize = txtMaxSliceSize->GetValue();
	encoder->IntraRefreshType = (IntraRefreshTypes)cmbIntraRefreshType->SelectedIndex;
	encoder->IRDist = txtIRDist->GetValue();
	encoder->IRCycle = txtIRCycle->GetValue();
}

void EncoderSample::EncoderSettingsControl::SetCodecType(CodecTypes type)
{
	if (type == CodecTypes::HEVC)
	{
		codecType = CodecTypes::HEVC;
		lblCodec->Text = "Codec: HEVC";
		if (reinterpret_cast<ComboBoxItem^>(cmbBRC->SelectedItem)->Content->ToString() == "LookAhead")
		{
			cmbBRC->SelectedIndex = 1;
		}
		cmbiLookAhead->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		lblLowDelayBRC->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		chkLowDelayBRC->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	else
	{
		codecType = CodecTypes::AVC;
		lblCodec->Text = "Codec: AVC";
		cmbiLookAhead->Visibility = Windows::UI::Xaml::Visibility::Visible;
		lblLowDelayBRC->Visibility = Windows::UI::Xaml::Visibility::Visible;
		chkLowDelayBRC->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}
}


void EncoderSample::EncoderSettingsControl::cmbBRC_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	if (QVBRBlock && LABlock && CQPBlock)
	{
		QVBRBlock->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		LABlock->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		CQPBlock->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

		String^ brcMode =  reinterpret_cast<ComboBoxItem^>(cmbBRC->SelectedValue)->Content->ToString();

		if (brcMode == L"QVBR")
		{
			QVBRBlock->Visibility = Windows::UI::Xaml::Visibility::Visible;
		}
		else if (brcMode == L"LookAhead")
		{
			LABlock->Visibility = Windows::UI::Xaml::Visibility::Visible;
		}
		else if (brcMode == L"CQP")
		{
			CQPBlock->Visibility = Windows::UI::Xaml::Visibility::Visible;
		}
	
		lblBitrate->Visibility = brcMode == L"CQP" ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
		txtBitrate->Visibility = lblBitrate->Visibility;

		lblWinBRCMaxAvgKbps->Visibility = brcMode == L"CQP" || brcMode == L"CBR" ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
		txtWinBRCMaxAvgKbps->Visibility = lblWinBRCMaxAvgKbps->Visibility;
		lblWinBRCSize->Visibility = lblWinBRCMaxAvgKbps->Visibility;
		txtWinBRCSize->Visibility = lblWinBRCMaxAvgKbps->Visibility;
		lblMaxKbps->Visibility = lblWinBRCMaxAvgKbps->Visibility;
		txtMaxKbps->Visibility = lblWinBRCMaxAvgKbps->Visibility;
	}
}


void EncoderSample::EncoderSettingsControl::cmbIntraRefreshType_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	if (IRBlock)
	{
		IRBlock->Visibility = cmbIntraRefreshType->SelectedIndex == 0 ? Windows::UI::Xaml::Visibility::Collapsed : Windows::UI::Xaml::Visibility::Visible;
	}
}