// Copyright (c) 2020-2022 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.Threading;
using System.IO;
using Microsoft.Win32;
using System.Reflection;
using System.Security.AccessControl;

namespace msdk_analyzer
{
    public static class _PATH
    {
       public static string TRACER_PATH = "\\Intel\\SDK_Tracer\\";
       public static string TRACER_LOG = "tracer.log";
    };

    public partial class SdkAnalyzerForm : Form
    {
        IDataCollector m_collector;
        FileInfo file;
        
        public SdkAnalyzerForm()
        {
            InitializeComponent();
            m_collector = new DataCollector();
            System.IO.Directory.CreateDirectory(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + _PATH.TRACER_PATH);
            tbxLogOutput.Text = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + _PATH.TRACER_PATH + _PATH.TRACER_LOG;
            m_collector.SetLog(tbxLogOutput.Text);
            m_collector.Create();

            FileInfo log_path = new FileInfo(tbxLogOutput.Text);
            if (log_path.Exists)
            {
                button_Open.Enabled = true;
                btnDeleteLog.Enabled = true;
            }
            else
            {
                button_Open.Enabled = false;
                btnDeleteLog.Enabled = false;
            }
        
        }

        void OnUpdateEtlFileSize(object sender, EventArgs e)
        {
            //
        }

        void  converter_ConversionUpdated(ConversionStatus sts, int percentage)
        {
             //
        }

        private void button_Start_Click(object sender, EventArgs e)
        {
            //button disabled since running a collector might take significant time
            
            if (m_collector.isRunning)
            {
                
                OnStop();
            }
            else
            {
                if (checkBox_Append.Checked != true)
                {
                    try
                    {
                        File.Delete(tbxLogOutput.Text);
                        lblBytesWritten.Text = "0";
                    }
                    catch (System.Exception ex)
                    {
                        MessageBox.Show(ex.ToString());
                    }
                }
                OnStart();
            }
        }

        private void OnStart()
        {
            button_Start.Enabled = false;
            checkBox_PerFrame.Enabled = false;
            checkBox_Append.Enabled = false;
            button_Open.Enabled = false;
            btnDeleteLog.Enabled = false;

            try
            {
                int nLevel =
                    checkBox_PerFrame.Checked ? 2 : 1;
                m_collector.SetLevel(nLevel);
                m_collector.SetLog(tbxLogOutput.Text);
                this.Cursor = Cursors.WaitCursor;
                m_collector.Start();
                MsdkAnalyzerCpp.start();

                this.Cursor = Cursors.Default;
                button_Start.Text = "Stop";
                timer1.Enabled = true;
                timer1.Start();
                               
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.ToString());
                checkBox_PerFrame.Enabled = true;
                checkBox_Append.Enabled = true;
            }
            button_Start.Enabled = true;
        }

        private void OnStop()
        {
            timer1.Stop();
            timer1.Enabled = false;
            button_Start.Enabled = false;

            try
            {
                this.Cursor = Cursors.WaitCursor;
                m_collector.Stop();
                MsdkAnalyzerCpp.stop();
                RunConversionDialog();
                button_Start.Text = "Start";
                button_Start.Enabled = true;
                checkBox_PerFrame.Enabled = true;
                checkBox_Append.Enabled = true;
                
                FileInfo log_path = new FileInfo(tbxLogOutput.Text);
                if (log_path.Exists)
                {
                    button_Open.Enabled = true;
                    btnDeleteLog.Enabled = true;
                }
                else
                {
                    button_Open.Enabled = false;
                    btnDeleteLog.Enabled = false;
                }
                this.Cursor = Cursors.Default;
                
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.ToString());
                button_Start.Text = "Start";
                button_Start.Enabled = true;
                checkBox_PerFrame.Enabled = true;
                checkBox_Append.Enabled = true;
                
                FileInfo log_path = new FileInfo(tbxLogOutput.Text);
                if (log_path.Exists)
                {
                    button_Open.Enabled = true;
                    btnDeleteLog.Enabled = true;
                }
                else
                {
                    button_Open.Enabled = false;
                    btnDeleteLog.Enabled = false;
                }
                this.Cursor = Cursors.Default;
            }
            
        }

        private void button_Open_Click(object sender, EventArgs e)
        {
           try
           {
               Process.Start(tbxLogOutput.Text);
           }
           catch (System.Exception ex)
           {
               MessageBox.Show(ex.ToString());
           }
           
        }

        private void Form4_FormClosing(object sender, FormClosingEventArgs e)
        {
            m_collector.Delete();
        }

        protected void RunConversionDialog()
        {
            //
        }

        
        private void OnConvertionFinished()
        {
            
            button_Start.Enabled = true;
            button_Start.Text = "Start";
            this.Cursor = Cursors.Default;
        }

        private void btnDeleteLog_Click(object sender, EventArgs e)
        {
            try
            {
                File.Delete(tbxLogOutput.Text);
                button_Open.Enabled = false;
                btnDeleteLog.Enabled = false;
                lblBytesWritten.Text = "0";
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
            
        }


        private void timer1_Tick_1(object sender, EventArgs e)
        {
            file = new FileInfo(tbxLogOutput.Text);
            if (file.Exists)
                lblBytesWritten.Text = (file.Length / 1000).ToString();
        }

    }
}
