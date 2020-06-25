﻿/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2020 Intel Corporation. All Rights Reserved.

File Name: EtlDataCollector.cs

\* ****************************************************************************** */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;

namespace msdk_analyzer
{
    class CollectorStat 
    {
        UInt64 m_bytesWritten;
        public UInt64 KBytesWritten
        {
            get { return m_bytesWritten; }
            set { m_bytesWritten = value; }
        }
    }

    interface IDataCollector
    {
        void SetLevel(int level);
        void SetLog(string log);
        void Start();
        void Stop();
        void Create();
        void Delete();
        void Query(CollectorStat stat);
        bool isRunning{ get; }
        string TargetFile { get; }
    }

    class DataCollector
        : IDisposable
        , IDataCollector
    {
        //TODO: questionalble variable might  be used to reduce collector update calls
       
        private bool m_bCollecting;
        private int m_level;
        private string m_log;
        private ConfigManager m_conf = new ConfigManager();
        private string config_path = Environment.CurrentDirectory + "\\.mfxtracer";
        //temporary filename
        private const string etl_file_name = "provider.etl";
        private const string etl_file_name_actual = "provider_000001.etl";

        public DataCollector()
        {
            //stop because if existed collector is running we cant delete it
            Stop();
        }

        public void Dispose()
        {
            Stop();
            Delete();
        }
        private string GetSdkAnalyzerDataFolder()
        {
            return Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData)
                + "\\" + Properties.Resources.app_data_folder;
        }

        public string TargetFile
        {
            get
            {
                return "\""+GetSdkAnalyzerDataFolder() + etl_file_name_actual+"\"";
            }
        }

        private Dictionary<string, Dictionary<string, string>> GetDefaultParam()
        {
            //set level
            string level = "default";
            if (m_level == 2)
            {
                level = "full";
            }
            else if (m_level == 1)
            {
                level = "default";
            }
            
            Dictionary<string, string> tmp_default_section = new Dictionary<string, string>();
            tmp_default_section.Add("level", level.ToString());
            tmp_default_section.Add("log", m_log);
            tmp_default_section.Add("type", "file");
            tmp_default_section.Add("lib", "none");

            Dictionary<string, Dictionary<string, string>> tmp_default_param = new Dictionary<string, Dictionary<string, string>>();
            tmp_default_param.Add("core", tmp_default_section);
            return tmp_default_param;
        }

        public void Create()
        {
            //session might exist from abnormal termination
            Delete();
            
            //create target folder if not exist
            string path = Path.GetDirectoryName(GetSdkAnalyzerDataFolder());
            Directory.CreateDirectory(path);

            // create config file
            Dictionary<string, Dictionary<string, string>> default_param = GetDefaultParam();
            m_conf.CreateConfig(config_path, default_param);
                      
        }

        public void SetLevel(int level)
        {
            m_level = level;
        }

        public void SetLog(string log)
        {
            m_log = "\"" + log + "\"";
        }

        public void Start()
        {
            //have to call to cr/eate to not spawn number of etl files
            Create();
                       
            m_bCollecting = true;
        }

        public void Stop()
        {
            m_bCollecting = false;
        }

        public void Delete()
        {
            Stop();
        }

        public void Query(CollectorStat statistics)
        {
            EVENT_TRACE_PROPERTIES prop = new EVENT_TRACE_PROPERTIES();
            UInt64 ret;
            prop.Wnode.BufferSize = 2048;
            
            ret = AdvApi.QueryTrace(0, Properties.Resources.msdk_analyzer_session_name, prop);
            
            statistics.KBytesWritten = prop.BufferSize * prop.BuffersWritten;
        }

        public bool isRunning
        {
            get
            {
                return m_bCollecting;
            }
        }
     
    }
}
