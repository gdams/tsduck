//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2022, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsVVCVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"VVC_video_descriptor"
#define MY_CLASS ts::VVCVideoDescriptor
#define MY_DID ts::DID_VVC_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VVCVideoDescriptor::VVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    profile_idc(0),
    tier(false),
    sub_profile_idc(),
    progressive_source(false),
    interlaced_source(false),
    non_packed_constraint(false),
    frame_only_constraint(false),
    level_idc(0),
    VVC_still_present(false),
    VVC_24hr_picture_present(false),
    HDR_WCG_idc(3),
    video_properties_tag(0),    
    temporal_id_min(),
    temporal_id_max()
{
}

void ts::VVCVideoDescriptor::clearContent()
{
    profile_idc = 0;
    tier = false;
    sub_profile_idc.clear(),
    progressive_source = false;
    interlaced_source = false;
    non_packed_constraint = false;
    frame_only_constraint = false;
    level_idc = 0;
    VVC_still_present = false;
    VVC_24hr_picture_present = false;
    HDR_WCG_idc = 3;
	video_properties_tag = 0;
    temporal_id_min.clear();
    temporal_id_max.clear();
}

ts::VVCVideoDescriptor::VVCVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    VVCVideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VVCVideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(profile_idc, 7);
    buf.putBit(tier);
    buf.putUInt8((uint8_t)sub_profile_idc.size());
    for (auto it = sub_profile_idc.begin(); it != sub_profile_idc.end(); ++it) {
        buf.putUInt32(*it);
    }
    buf.putBit(progressive_source);
    buf.putBit(interlaced_source);
    buf.putBit(non_packed_constraint);
    buf.putBit(frame_only_constraint);
    buf.putBits(0x00, 4);
    buf.putUInt8(level_idc);
    const bool temporal_layer_subset_flag = temporal_id_min.set() && temporal_id_max.set();
    buf.putBit(temporal_layer_subset_flag);
    buf.putBit(VVC_still_present);
    buf.putBit(VVC_24hr_picture_present);
    buf.putBits(0xFF, 5);
    buf.putBits(HDR_WCG_idc, 2);
    buf.putBits(0xFF, 2);
    buf.putBits(video_properties_tag, 4);
    if (temporal_layer_subset_flag) {
        buf.putBits(0xFF, 5);
        buf.putBits(temporal_id_min.value(), 3);
        buf.putBits(0xFF, 5);
        buf.putBits(temporal_id_max.value(), 3);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VVCVideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(profile_idc, 7);
    tier = buf.getBool();
    uint8_t num_sub_profiles = buf.getUInt8();
    for (uint8_t i = 0; i < num_sub_profiles; i++)
        sub_profile_idc.push_back(buf.getUInt32());
    progressive_source = buf.getBool();
    interlaced_source = buf.getBool();
    non_packed_constraint = buf.getBool();
    frame_only_constraint = buf.getBool();
    buf.skipBits(4);
    level_idc = buf.getUInt8();
    const bool temporal_layer_subset_flag = buf.getBool();
    VVC_still_present = buf.getBool();
    VVC_24hr_picture_present = buf.getBool();
    buf.skipBits(5);
    buf.getBits(HDR_WCG_idc, 2);
    buf.skipBits(2);
    buf.getBits(video_properties_tag, 4);
    if (temporal_layer_subset_flag) {
        buf.skipBits(5);
        buf.getBits(temporal_id_min, 3);
        buf.skipBits(5);
        buf.getBits(temporal_id_max, 3);
    }
}

//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

std::string VVCProfileIDC(INT pi) {
	switch (pi) {
		case  1: return "Main 10";
		case 17: return "Multilayer Main 10";
		case 33: return "Main 10 4:4:4";
		case 49: return "Multilayer Main 10 4:4:4 ";
		case 65: return "Main 10 Still Picture";
		case 97: return "Main 10 4:4:4 Still Picture";
        default: return "unknown";
	}
}

std::string VVCTier(bool t) {
	return t ? "High" : "Main";
}

std::string VVCHDRandWCG(INT hw) {
	// H222.0, TAble 2-134
    switch (hw) {
		case 0: return "SDR";
		case 1: return "WCG only";
		case 2: return "HDR and WCG";
		case 3: return "no indication";
        default: return "unknown";
	}	
}

std::string VVCLevelIDC(INT li) {
	// H.266, Table A.1
	switch (li) {
		case  16: return "1.0";
		case  32: return "2.0";
		case  35: return "2.1";
		case  48: return "3.0";
		case  51: return "3.1";
		case  64: return "4.0";
		case  67: return "4.1";
		case  80: return "5.0";
		case  83: return "5.1";
		case  86: return "5.2";
		case  96: return "6.0";
		case  99: return "6.1";
		case 102: return "6.2";
        default: return "unknown";
	}
}

std::string VVCVideoProperties(INT vp) {
	// H.222.0 Table 2-135
	switch (vp) {
		case 0: return "not known";
		case 1: return "BT709_YCC";
		case 2: return "BT709_RGB";
		case 3: return "BT601_525";
		case 4: return "BT601_625";
		case 5: return "FR709_RGB";
        default: return "unknown";
	}
}

void ts::VVCVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(8)) {
        INT t;
        t = buf.getBits<uint16_t>(7);
        disp << margin << "Profile IDC: " << VVCProfileIDC(t) << " (" << UString::Hexa(t, 2);
        disp << "), tier: " << VVCTier(buf.getBool()) << std::endl;
        uint8_t num_sub_profiles = buf.getUInt8();
        if (num_sub_profiles > 0) {
            disp << margin << "Sub profile IDC: ";
            for (uint8_t i = 0; i < num_sub_profiles; i++) {
                disp << UString::Hexa(buf.getUInt32()) << " ";
                if ((i + 1) % 6 == 0) {
                    disp << std::endl;
                    if (i != (num_sub_profiles - 1))
                        disp << margin << "                 ";
                }
            }
        }
        disp << margin << "Progressive source: " << UString::TrueFalse(buf.getBool());
        disp << ", interlaced source: " << UString::TrueFalse(buf.getBool());
        disp << ", non packed: " << UString::TrueFalse(buf.getBool());
        disp << ", frame only: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(4);
        t=buf.getUInt8();
        disp << margin << "Level IDC: " << VVCLevelIDC(t) << " (" << UString::Hexa(t, 2) << ")";
        const bool temporal = buf.getBool();
        disp << ", still pictures: " << UString::TrueFalse(buf.getBool());
        disp << ", 24-hour pictures: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(5);
        t=buf.getBits<uint16_t>(2);
        disp << margin << "HDR WCG idc: " << VVCHDRandWCG(t) << " (" << t << ")";
        buf.skipBits(2);
		t=buf.getBits<uint16_t>(4);
        disp << ", video properties: " << VVCVideoProperties(t) << "(" << t << ")" << std::endl;
        if (temporal && buf.canReadBytes(2)) {
            buf.skipBits(5);
            disp << margin << "Temporal id min: " << buf.getBits<uint16_t>(3);
            buf.skipBits(5);
            disp << ", max: " << buf.getBits<uint16_t>(3) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VVCVideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{ 
    root->setIntAttribute(u"profile_idc", profile_idc, true);
    root->setBoolAttribute(u"tier_flag", tier);
    root->setIntAttribute(u"num_sub_profiles", sub_profile_idc.size());
    for (auto it = sub_profile_idc.begin(); it != sub_profile_idc.end(); ++it) {
        uint32_t sub_profile_idc_val = *it;
        root->addHexaTextChild(u"sub_profile_idc", &sub_profile_idc_val, sizeof(uint32_t));
    }
    root->setBoolAttribute(u"progressive_source_flag", progressive_source);
    root->setBoolAttribute(u"interlaced_source_flag", interlaced_source);
    root->setBoolAttribute(u"non_packed_constraint_flag", non_packed_constraint);
    root->setBoolAttribute(u"frame_only_constraint_flag", frame_only_constraint);
    root->setIntAttribute(u"level_idc", level_idc, true);
    root->setBoolAttribute(u"temporal_layer_subset_flag", temporal_id_min.set() && temporal_id_max.set());
    root->setBoolAttribute(u"VVC_still_present_flag", VVC_still_present);
    root->setBoolAttribute(u"VVC_24hr_picture_present_flag", VVC_24hr_picture_present);
    root->setIntAttribute(u"HDR_WCG_idc", HDR_WCG_idc);
    root->setIntAttribute(u"video_properties_tag", video_properties_tag);
    root->setOptionalIntAttribute(u"temporal_id_min", temporal_id_min);
    root->setOptionalIntAttribute(u"temporal_id_max", temporal_id_max);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VVCVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    uint8_t num_sub_profiles = 0;
    bool ok =
        element->getIntAttribute(profile_idc, u"profile_idc", true, 0, 0x00, 0x1F) &&
        element->getBoolAttribute(tier, u"tier_flag", true) &&
        element->getIntAttribute(num_sub_profiles, u"num_sub_profiles", false, 0, 0x00, 0xFF) &&
        element->getBoolAttribute(progressive_source, u"progressive_source_flag", true) &&
        element->getBoolAttribute(interlaced_source, u"interlaced_source_flag", true) &&
        element->getBoolAttribute(non_packed_constraint, u"non_packed_constraint_flag", true) &&
        element->getBoolAttribute(frame_only_constraint, u"frame_only_constraint_flag", true) &&
        element->getIntAttribute(level_idc, u"level_idc", true) &&
        element->getBoolAttribute(VVC_still_present, u"VVC_still_present_flag", true) &&
        element->getBoolAttribute(VVC_24hr_picture_present, u"VVC_24hr_picture_present_flag", true) &&
        element->getIntAttribute(HDR_WCG_idc, u"HDR_WCG_idc", false, 3, 0, 3) &&
        element->getIntAttribute(video_properties_tag, u"video_properties_tag", false, 0, 0, 15);

    if (ok) {
        xml::ElementVector children;
        ok &= element->getChildren(children, u"sub_profile_idc");

        for (size_t i = 0; ok && i < children.size(); ++i) {
            UString hexVal(u"");
            ok &= children[i]->getText(hexVal);
            INT val = INT(0);
            if (!hexVal.toInteger(val, u",")) {
                element->report().error(u"'%s' is not a valid integer value for attribute '%s' in <%s>, line %d", { hexVal, u"sub_profile_idc", element->lineNumber(), element->name() });
                ok = false;
            }
            else if (val < INT(0) || val > INT(0xFFFF)) {
                element->report().error(u"'%s' must be in range %'d to %'d for attribute '%s' in <%s>, line %d", { hexVal, 0, 0xFFFF, u"sub_profile_idc", element->lineNumber(), element->name() });
                ok = false;
            }
        }
    }

    if (ok && temporal_id_min.set() + temporal_id_max.set() == 1) {
        element->report().error(u"line %d: in <%s>, attributes 'temporal_id_min' and 'temporal_id_max' must be both present or both omitted", { element->lineNumber(), element->name() });
        ok = false;
    }

    return ok;
}
