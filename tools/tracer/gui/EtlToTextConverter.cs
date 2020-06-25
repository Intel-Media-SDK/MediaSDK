/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2020 Intel Corporation. All Rights Reserved.

File Name: 

\* ****************************************************************************** */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Runtime.InteropServices;

namespace msdk_analyzer
{
    enum ConversionStatus
    {
        CONVERSION_STARTED,
        CONVERSION_PROGRESS,
        CONVERSION_ABORTED,
        CONVERSION_COMPLETED
    }

    delegate void ConversionStatusUpdated(ConversionStatus sts, int percentage);

    class EtlToTextConverter
    {
        uint m_percentage;
        ConversionStatus m_status;
        string m_etlfilename;
        string m_textfilename;
        
        public EtlToTextConverter()
        {
        }
        /// <summary>
        /// Does Conversion from etl file to user readable text form
        /// </summary>
        /// <param name="etlfilename"></param>
        /// <param name="textfilename"></param>
        public void StartConvert(string etlfilename, string textfilename)
        {
            m_etlfilename = etlfilename;
            m_textfilename = textfilename;
            ThreadPool.QueueUserWorkItem(RunConversion);
        }

        private void RunConversion(object state)
        {
            m_status = ConversionStatus.CONVERSION_STARTED;
            m_percentage = 50;
            MsdkAnalyzerCpp.convert_etl_to_text(0, 0, m_etlfilename + " " + m_textfilename, 1);
            m_percentage = 100;
            m_status = ConversionStatus.CONVERSION_COMPLETED;
        }
        
        public void GetConversionStatus(ref ConversionStatus sts, ref uint percentage)
        {
            sts = m_status;
            percentage = m_percentage;
        }
    }
}
