//test_verif.cpp - verification blocks
//prefix "v_" for each block
#include "stdafx.h"
#include "test_common.h"
#include <list>

msdk_ts_BLOCK(v_CheckField){
    switch(var_def<mfxU8>("field_size", 4)){
        case 1:
            CHECK(var_old<mfxU8>("field", var_def<mfxU32>("field_offset", 0)) == var_old<mfxU8>("field_ref"), 
                var_old<char*>("field_name") << " = " << var_old<mfxU8>("field", var_old<mfxU32>("field_offset")) << 
                ", expected "    << var_old<mfxU8>("field_ref"));
            break;
        case 2:
            CHECK(var_old<mfxU16>("field", var_def<mfxU32>("field_offset", 0)) == var_old<mfxU16>("field_ref"), 
                var_old<char*>("field_name") << " = " << var_old<mfxU16>("field", var_old<mfxU32>("field_offset")) << 
                ", expected "    << var_old<mfxU16>("field_ref"));
            break;
        case 4:
            CHECK(var_old<mfxU32>("field", var_def<mfxU32>("field_offset", 0)) == var_old<mfxU32>("field_ref"), 
                var_old<char*>("field_name") << " = " << var_old<mfxU32>("field", var_old<mfxU32>("field_offset")) << 
                ", expected "    << var_old<mfxU32>("field_ref"));
            break;
        case 8:
            CHECK(var_old<mfxU64>("field", var_def<mfxU32>("field_offset", 0)) == var_old<mfxU64>("field_ref"), 
                var_old<char*>("field_name") << " = " << var_old<mfxU64>("field", var_old<mfxU32>("field_offset")) << 
                ", expected "    << var_old<mfxU64>("field_ref"));
            break;
        default: 
            std::cout << "ERR: illegal field size: " << var_old<mfxU8>("field_size");
            return msdk_ts::resINVPAR;
    }
    return msdk_ts::resOK;
}

msdk_ts_BLOCK(v_CheckField_NotEqual){
    switch(var_def<mfxU8>("field_size", 4)){
        case 1:
            CHECK(var_old<mfxU8>("field", var_def<mfxU32>("field_offset", 0)) != var_old<mfxU8>("field_ref"), 
                var_old<char*>("field_name") << " = " << var_old<mfxU8>("field", var_old<mfxU32>("field_offset")) << 
                ", expected !="    << var_old<mfxU8>("field_ref"));
            break;
        case 2:
            CHECK(var_old<mfxU16>("field", var_def<mfxU32>("field_offset", 0)) != var_old<mfxU16>("field_ref"), 
                var_old<char*>("field_name") << " = " << var_old<mfxU16>("field", var_old<mfxU32>("field_offset")) << 
                ", expected !="    << var_old<mfxU16>("field_ref"));
            break;
        case 4:
            CHECK(var_old<mfxU32>("field", var_def<mfxU32>("field_offset", 0)) != var_old<mfxU32>("field_ref"), 
                var_old<char*>("field_name") << " = " << var_old<mfxU32>("field", var_old<mfxU32>("field_offset")) << 
                ", expected !="    << var_old<mfxU32>("field_ref"));
            break;
        case 8:
            CHECK(var_old<mfxU64>("field", var_def<mfxU32>("field_offset", 0)) != var_old<mfxU64>("field_ref"), 
                var_old<char*>("field_name") << " = " << var_old<mfxU64>("field", var_old<mfxU32>("field_offset")) << 
                ", expected !="    << var_old<mfxU64>("field_ref"));
            break;
        default: 
            std::cout << "ERR: illegal field size: " << var_old<mfxU8>("field_size");
            return msdk_ts::resINVPAR;
    }
    return msdk_ts::resOK;
}

msdk_ts_BLOCK(v_CheckMfxRes){
    mfxStatus mfxRes      = var_old<mfxStatus>("mfxRes"     );
    mfxStatus expectedRes = var_old<mfxStatus>("expectedRes");
    CHECK_STS(mfxRes, expectedRes);
    return msdk_ts::resOK;
}

/*
bool isSliceHeader(Bs8u nalu_type){
    return (nalu_type == 1 || nalu_type == 5 || nalu_type == 20);
}

msdk_ts_BLOCK(v_CheckAVCBPyramid){
    BS_H264_au& au = var_old<BS_H264_au>("h264_au_hdr");
    std::list<mfxI32>& dpb_reorder = var_old_or_new<std::list<mfxI32> >("dpb_reorder");
    mfxU32& last_frame_poc = var_def<mfxU32>("last_frame_poc", 0);

    for(BS_H264_au::nalu_param* nalu = au.NALU; nalu < (au.NALU+au.NumUnits); nalu++){
        if(!isSliceHeader(nalu->nal_unit_type)) continue;
        if(nalu->slice_hdr->first_mb_in_slice)  continue;

        slice_header& frame = *nalu->slice_hdr;
        bool num_reorder_frames_set = false;

        dpb_reorder.push_back(frame.PicOrderCnt);
        dpb_reorder.sort(std::less<mfxI32>());
        while(dpb_reorder.size() && (dpb_reorder.front() == 0 || (dpb_reorder.front() - last_frame_poc) < 3)){
            last_frame_poc = dpb_reorder.front();
            dpb_reorder.pop_front();
        }

        if(frame.sps_active->vui_parameters_present_flag){
            if(frame.sps_active->vui->bitstream_restriction_flag){
                num_reorder_frames_set = true;
                CHECK(frame.sps_active->vui->num_reorder_frames >= dpb_reorder.size(), "num_reorder_frames value is wrong");
            }
        }
        CHECK(num_reorder_frames_set, "num_reorder_frames is not set");

        if(nalu->slice_hdr->slice_type%5 != 1)  continue;
        if(nalu->slice_hdr->isReferencePicture) continue;

        Bs32u l0_idx = 1 + ( frame.num_ref_idx_active_override_flag ? frame.num_ref_idx_l0_active_minus1 : frame.pps_active->num_ref_idx_l0_default_active_minus1 );
        Bs32u l1_idx = 1 + ( frame.num_ref_idx_active_override_flag ? frame.num_ref_idx_l1_active_minus1 : frame.pps_active->num_ref_idx_l1_default_active_minus1 );
        bool b_pyramid = false;

        while(l0_idx--){
            CHECK(frame.RefPicList[0][l0_idx], "incomplete RefPicList0");
            if(    frame.prev_ref_pic == frame.RefPicList[0][l0_idx]
                && frame.prev_ref_pic->slice_type%5 == 1
                && frame.PicOrderCnt > frame.prev_ref_pic->PicOrderCnt)
                    b_pyramid = true;
        }
        while(l1_idx--){
            CHECK(frame.RefPicList[1][l1_idx], "incomplete RefPicList1");
            if(    frame.prev_ref_pic == frame.RefPicList[1][l1_idx]
                && frame.prev_ref_pic->slice_type%5 == 1
                && frame.PicOrderCnt < frame.prev_ref_pic->PicOrderCnt)
                    b_pyramid = true;
        }
        CHECK(b_pyramid, "B-pyramyd expected");
    }

    return msdk_ts::resOK;
}
*/

msdk_ts_BLOCK(v_CheckMfxExtVPPDoUse){
    mfxExtVPPDoUse& vppDoUse = var_old<mfxExtVPPDoUse>("mfxvppdouse");
    mfxU32 reqAlgm = var_old_or_new<mfxU32>("req_algm");

    CHECK(vppDoUse.NumAlg, "Number of VPP do use algorithms is 0");
    for( mfxU32 iii = 0; iii < vppDoUse.NumAlg; iii++ )
    {
        if( reqAlgm == vppDoUse.AlgList[iii] )
        {
            return msdk_ts::resOK;
        }
    }
    std::cout << "ERR: required VPP algorithm was not found\n";
    return msdk_ts::resFAIL;
}
