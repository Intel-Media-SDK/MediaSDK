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

namespace msdk_analyzer
{
    partial class SdkAnalyzerForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.label1 = new System.Windows.Forms.Label();
            this.tbxLogOutput = new System.Windows.Forms.TextBox();
            this.checkBox_PerFrame = new System.Windows.Forms.CheckBox();
            this.button_Open = new System.Windows.Forms.Button();
            this.button_Start = new System.Windows.Forms.Button();
            this.btnDeleteLog = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.lblBytesWritten = new System.Windows.Forms.Label();
            this.checkBox_Append = new System.Windows.Forms.CheckBox();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(18, 27);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(58, 13);
            this.label1.TabIndex = 11;
            this.label1.Text = "Output File";
            // 
            // tbxLogOutput
            // 
            this.tbxLogOutput.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbxLogOutput.Location = new System.Drawing.Point(99, 24);
            this.tbxLogOutput.Name = "tbxLogOutput";
            this.tbxLogOutput.Size = new System.Drawing.Size(390, 20);
            this.tbxLogOutput.TabIndex = 10;
            // 
            // checkBox_PerFrame
            // 
            this.checkBox_PerFrame.AutoSize = true;
            this.checkBox_PerFrame.Location = new System.Drawing.Point(82, 116);
            this.checkBox_PerFrame.Name = "checkBox_PerFrame";
            this.checkBox_PerFrame.Size = new System.Drawing.Size(108, 17);
            this.checkBox_PerFrame.TabIndex = 15;
            this.checkBox_PerFrame.Text = "Per-frame logging";
            this.checkBox_PerFrame.UseVisualStyleBackColor = true;
            // 
            // button_Open
            // 
            this.button_Open.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.button_Open.DialogResult = System.Windows.Forms.DialogResult.No;
            this.button_Open.Location = new System.Drawing.Point(364, 109);
            this.button_Open.Margin = new System.Windows.Forms.Padding(2);
            this.button_Open.Name = "button_Open";
            this.button_Open.Size = new System.Drawing.Size(56, 28);
            this.button_Open.TabIndex = 13;
            this.button_Open.Text = "Open";
            this.button_Open.UseVisualStyleBackColor = true;
            this.button_Open.Click += new System.EventHandler(this.button_Open_Click);
            // 
            // button_Start
            // 
            this.button_Start.Location = new System.Drawing.Point(21, 109);
            this.button_Start.Margin = new System.Windows.Forms.Padding(2);
            this.button_Start.Name = "button_Start";
            this.button_Start.Size = new System.Drawing.Size(56, 28);
            this.button_Start.TabIndex = 1;
            this.button_Start.Text = "Start";
            this.button_Start.UseVisualStyleBackColor = true;
            this.button_Start.Click += new System.EventHandler(this.button_Start_Click);
            // 
            // btnDeleteLog
            // 
            this.btnDeleteLog.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnDeleteLog.DialogResult = System.Windows.Forms.DialogResult.No;
            this.btnDeleteLog.Location = new System.Drawing.Point(424, 109);
            this.btnDeleteLog.Margin = new System.Windows.Forms.Padding(2);
            this.btnDeleteLog.Name = "btnDeleteLog";
            this.btnDeleteLog.Size = new System.Drawing.Size(65, 28);
            this.btnDeleteLog.TabIndex = 13;
            this.btnDeleteLog.Text = "Delete";
            this.btnDeleteLog.UseVisualStyleBackColor = true;
            this.btnDeleteLog.Click += new System.EventHandler(this.btnDeleteLog_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(19, 51);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(77, 13);
            this.label2.TabIndex = 11;
            this.label2.Text = "KBytes Written";
            // 
            // lblBytesWritten
            // 
            this.lblBytesWritten.Location = new System.Drawing.Point(99, 51);
            this.lblBytesWritten.Name = "lblBytesWritten";
            this.lblBytesWritten.Size = new System.Drawing.Size(91, 13);
            this.lblBytesWritten.TabIndex = 16;
            this.lblBytesWritten.Text = "0";
            // 
            // checkBox_Append
            // 
            this.checkBox_Append.AutoSize = true;
            this.checkBox_Append.Location = new System.Drawing.Point(196, 116);
            this.checkBox_Append.Name = "checkBox_Append";
            this.checkBox_Append.Size = new System.Drawing.Size(129, 17);
            this.checkBox_Append.TabIndex = 13;
            this.checkBox_Append.Text = "Append to existing file";
            this.checkBox_Append.UseVisualStyleBackColor = true;
            // 
            // timer1
            // 
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick_1);
            // 
            // SdkAnalyzerForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(501, 148);
            this.Controls.Add(this.checkBox_Append);
            this.Controls.Add(this.lblBytesWritten);
            this.Controls.Add(this.button_Start);
            this.Controls.Add(this.checkBox_PerFrame);
            this.Controls.Add(this.btnDeleteLog);
            this.Controls.Add(this.button_Open);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.tbxLogOutput);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "SdkAnalyzerForm";
            this.ShowIcon = false;
            this.Text = "SDK Tracer";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form4_FormClosing);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox tbxLogOutput;
        private System.Windows.Forms.CheckBox checkBox_PerFrame;
        private System.Windows.Forms.Button button_Open;
        private System.Windows.Forms.Button button_Start;
        private System.Windows.Forms.Button btnDeleteLog;
        private System.Windows.Forms.CheckBox checkBox_Append;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label lblBytesWritten;
        private System.Windows.Forms.Timer timer1;
    }
}
