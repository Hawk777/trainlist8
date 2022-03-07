#include "pch.h"
#include <array>
#include <cstddef>
#include <limits>
#include <type_traits>
#include "soap.h"
#include "util.h"

namespace soap = trainlist8::soap;
using trainlist8::util::operator""_as_xml;

namespace trainlist8::soap {
namespace {
// Constants used by multiple messages.
namespace common {
// A struct with no members.
constinit const WS_STRUCT_DESCRIPTION emptyStruct = {
	.alignment = 1,
};

// The pMessage element name.
constinit const WS_XML_STRING pMessageFieldName = u8"pMessage"_as_xml;

// The http://tempuri.org/ URL.
constinit const WS_XML_STRING tempURI = u8"http://tempuri.org/"_as_xml;

// The http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8 URL.
constinit const WS_XML_STRING messagesFromRun8 = u8"http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8"_as_xml;

// A struct field that matches and discards any single element.
constinit const WS_FIELD_DESCRIPTION discardedElementField = {
	.mapping = WS_ANY_ELEMENT_FIELD_MAPPING,
	.type = WS_VOID_TYPE,
};

// A struct description for an element that contains a child element named "pMessage" which is discarded.
constinit const std::array discardedStructFields = {
	const_cast<WS_FIELD_DESCRIPTION *>(&discardedElementField),
};
constinit const WS_STRUCT_DESCRIPTION discardedStruct = {
	.alignment = 1,
	.fields = const_cast<WS_FIELD_DESCRIPTION **>(discardedStructFields.data()),
	.fieldCount = discardedStructFields.size(),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/DispatcherConnected action
//
// This action has an empty root element.
namespace dispatcherConnected {
// The root DispatcherConnected XML element.
constinit const WS_XML_STRING rootElementName = u8"DispatcherConnected"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/DispatcherConnected"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootElementName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&common::emptyStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/DTMF action
//
// This action has a body of the following format:
//
// <DTMF xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:Channel>55</b:Channel>
//		<b:DTMFType>None</b:DTMFType>
//		<b:Tone>*41</b:Tone>
//		<b:TowerDescription>BNSF_Shirley_Tower</b:TowerDescription>
//	</pMessage>
// </DTMF>
// 
// In this application, the body is ignored.
namespace dtmf {
constinit const WS_XML_STRING rootName = u8"DTMF"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/DTMF"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&common::discardedStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/PermissionUpdate action
//
// This action has a body of the following format:
// <PermissionUpdate xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:AIPermission>false</b:AIPermission>
//		<b:Permission>Rescinded</b:Permission
//	</pMessage>
// </PermissionUpdate>
namespace permissionUpdate {
// Sanity checks on the C++-side structure.
static_assert(std::is_standard_layout_v<DispatcherPermission>);
static_assert(std::is_trivial_v<DispatcherPermission>);

// The DispatcherPermission enum.
constexpr const WS_XML_STRING dispatcherPermissionGrantedString = u8"Granted"_as_xml;
constexpr const WS_XML_STRING dispatcherPermissionRescindedString = u8"Rescinded"_as_xml;
constexpr const WS_XML_STRING dispatcherPermissionObserverString = u8"Observer"_as_xml;
constexpr const WS_ENUM_VALUE dispatcherPermissionGrantedValue = {
	.value = static_cast<int>(DispatcherPermissionLevel::GRANTED),
	.name = const_cast<WS_XML_STRING *>(&dispatcherPermissionGrantedString),
};
constexpr const WS_ENUM_VALUE dispatcherPermissionRescindedValue = {
	.value = static_cast<int>(DispatcherPermissionLevel::RESCINDED),
	.name = const_cast<WS_XML_STRING *>(&dispatcherPermissionRescindedString),
};
constexpr const WS_ENUM_VALUE dispatcherPermissionObserverValue = {
	.value = static_cast<int>(DispatcherPermissionLevel::OBSERVER),
	.name = const_cast<WS_XML_STRING *>(&dispatcherPermissionObserverString),
};
constexpr const std::array dispatcherPermissionValues = {
	dispatcherPermissionGrantedValue,
	dispatcherPermissionRescindedValue,
	dispatcherPermissionObserverValue,
};
constinit const WS_ENUM_DESCRIPTION dispatcherPermissionEnumDescription = {
	.values = const_cast<WS_ENUM_VALUE *>(dispatcherPermissionValues.data()),
	.valueCount = dispatcherPermissionValues.size(),
	.maxByteCount = []() {
		unsigned long ret = 0;
		for(const WS_ENUM_VALUE &i : dispatcherPermissionValues) {
			ret = std::max(ret, i.name->length);
		}
		return ret;
}(),
};

// The AIPermission XML element.
constinit const WS_XML_STRING aiPermissionFieldName = u8"AIPermission"_as_xml;
constinit const WS_FIELD_DESCRIPTION aiPermissionFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&aiPermissionFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_BOOL_TYPE,
	.offset = offsetof(DispatcherPermission, aiPermission),
};

// The Permission XML element.
constinit const WS_XML_STRING permissionFieldName = u8"Permission"_as_xml;
constinit const WS_FIELD_DESCRIPTION permissionFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&permissionFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_ENUM_TYPE,
	.typeDescription = const_cast<WS_ENUM_DESCRIPTION *>(&dispatcherPermissionEnumDescription),
	.offset = offsetof(DispatcherPermission, permission),
};

// The pMessage XML element.
constinit const std::array pMessageFields = {
	const_cast<WS_FIELD_DESCRIPTION *>(&aiPermissionFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&permissionFieldDescription),
};
constinit const WS_STRUCT_DESCRIPTION pMessageStruct = {
	.size = sizeof(DispatcherPermission),
	.alignment = alignof(DispatcherPermission),
	.fields = const_cast<WS_FIELD_DESCRIPTION **>(pMessageFields.data()),
	.fieldCount = pMessageFields.size(),
};
constinit const WS_FIELD_DESCRIPTION pMessageFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&common::pMessageFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&pMessageStruct),
};

// The root PermissionUpdate XML element.
constinit const std::array rootFields = {
	const_cast<WS_FIELD_DESCRIPTION *>(&pMessageFieldDescription),
};
constinit const WS_STRUCT_DESCRIPTION rootStruct = {
	.size = sizeof(DispatcherPermission),
	.alignment = alignof(DispatcherPermission),
	.fields = const_cast<WS_FIELD_DESCRIPTION **>(rootFields.data()),
	.fieldCount = rootFields.size(),
};
constinit const WS_XML_STRING rootName = u8"PermissionUpdate"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/PermissionUpdate"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&rootStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/RadioText action
//
// This action has a body of presently unknown format.
//
// In this application, the body is ignored.
namespace radioText {
constinit const WS_XML_STRING rootName = u8"RadioText"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/RadioText"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&common::discardedStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/SendSimulationState action
//
// This action has a body of the following format:
//
// <SendSimulationState xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:IsClient>false</b:IsClient>
//		<b:SimulationTime>2017-09-14T14:03:17.5266626Z</b:SimulationTime>
//	</pMessage>
// </SendSimulationState>
namespace sendSimulationState {
// Sanity checks on the C++-side structure.
static_assert(std::is_standard_layout_v<SimulationState>);
static_assert(std::is_trivial_v<SimulationState>);

// The IsClient XML element.
constinit const WS_XML_STRING isClientFieldName = u8"IsClient"_as_xml;
constinit const WS_FIELD_DESCRIPTION isClientFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&isClientFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_BOOL_TYPE,
	.offset = offsetof(SimulationState, client),
};

// The SimulationTime XML element.
constinit const WS_XML_STRING simulationTimeFieldName = u8"SimulationTime"_as_xml;
constinit const WS_FIELD_DESCRIPTION simulationTimeFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&simulationTimeFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_DATETIME_TYPE,
	.offset = offsetof(SimulationState, time),
};

// The pMessage XML element.
constinit const std::array pMessageFieldDescriptions = {
	const_cast<WS_FIELD_DESCRIPTION *>(&isClientFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&simulationTimeFieldDescription),
};
constinit const WS_STRUCT_DESCRIPTION pMessageStructDescription = {
	.size = sizeof(SimulationState),
	.alignment = alignof(SimulationState),
	.fields = const_cast<WS_FIELD_DESCRIPTION **>(pMessageFieldDescriptions.data()),
	.fieldCount = pMessageFieldDescriptions.size(),
};
constinit const WS_FIELD_DESCRIPTION pMessageFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&common::pMessageFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&pMessageStructDescription),
};

// The root SendSimulationState XML element.
constinit const std::array rootFieldDescriptions = {
	const_cast<WS_FIELD_DESCRIPTION *>(&pMessageFieldDescription),
};
constinit const WS_XML_STRING rootName = u8"SendSimulationState"_as_xml;
constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/SendSimulationState"_as_xml;
constinit const WS_STRUCT_DESCRIPTION rootStructDescription = {
	.size = sizeof(SimulationState),
	.alignment = alignof(SimulationState),
	.fields = const_cast<WS_FIELD_DESCRIPTION **>(rootFieldDescriptions.data()),
	.fieldCount = rootFieldDescriptions.size(),
};
constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&rootStructDescription),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/SetInterlockErrorSwitches action
//
// This action has a body of the following format:
//
//<SetInterlockErrorSwitches xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:InterlockErrorSwitches xmlns:c="http://schemas.microsoft.com/2003/10/Serialization/Arrays">
//			<c:int>26</c:int>
//			…
//		</b:InterlockErrorSwitches>
//		<b:Route>250</b:Route>
//	</pMessage>
// </SetInterlockErrorSwitches>
// 
// In this application, the body is ignored.
namespace setInterlockErrorSwitches {
constinit const WS_XML_STRING rootName = u8"SetInterlockErrorSwitches"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/SetInterlockErrorSwitches"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&common::discardedStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/SetOccupiedBlocks action
//
// This action has a body of the following format:
//
// <SetOccupiedBlocks xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:OccupiedBlocks xmlns:c="http://schemas.microsoft.com/2003/10/Serialization/Arrays">
//			<c:int>170</c:int>
//			…
//		</b:OccupiedBlocks>
//		<b:OpenManualSwitchBlocks xmlns:c="http://schemas.microsoft.com/2003/10/Serialization/Arrays">
//			<c:int>124</c:int>
//			…
//		</b:OpenManualSwitchBlocks>
//		<b:Route>250</b:Route>
//	</pMessage>
// </SetOccupiedBlocks>
//
// In this application, the body is ignored.
namespace setOccupiedBlocks {
constinit const WS_XML_STRING rootName = u8"SetOccupiedBlocks"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/SetOccupiedBlocks"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&common::discardedStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/SetOccupiedSwitches action
//
// This action has a body of the following format:
//
// <SetOccupiedSwitches xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:OccupiedSwitches xmlns:c="http://schemas.microsoft.com/2003/10/Serialization/Arrays">
//			<c:int>65</c:int>
//			…
//		</b:OccupiedSwitches>
//		<b:Route>250</b:Route>
//	</pMessage>
// </SetOccupiedSwitches>
//
// In this application, the body is ignored.
namespace setOccupiedSwitches {
constinit const WS_XML_STRING rootName = u8"SetOccupiedSwitches"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/SetOccupiedSwitches"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&common::discardedStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/SetReversedSwitches action
// <SetReversedSwitches xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:ReversedSwitches xmlns:c="http://schemas.microsoft.com/2003/10/Serialization/Arrays">
//			<c:int>7</c:int>
//		</b:ReversedSwitches>
//		<b:Route>100</b:Route>
//	</pMessage>
// </SetReversedSwitches>
//
// In this application, the body is ignored.
namespace setReversedSwitches {
constinit const WS_XML_STRING rootName = u8"SetReversedSwitches"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/SetReversedSwitches"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&common::discardedStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/SetSignals action
//
// This action has a body of the following format:
// <SetSignals xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:Route>100</b:Route>
//		<b:Signals xmlns:c="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromDispatcher">
//			<c:ESignalIndication>Stop</c:ESignalIndication>
//			<c:ESignalIndication>Proceed</c:ESignalIndication>
//			<c:ESignalIndication>Fleet</c:ESignalIndication>
//			<c:ESignalIndication>FlagBy</c:ESignalIndication>
//			…
//		</b:Signals>
//	</pMessage>
// </SetSignals>
//
// In this application, the body is ignored.
namespace setSignals {
constinit const WS_XML_STRING rootName = u8"SetSignals"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/SetSignals"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&common::discardedStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/SetUnlockedSwitches action
//
// This action has a body of the following format:
// <SetUnlockedSwitches xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:Route>250</b:Route>
//		<b:UnlockedSwitches xmlns:c="http://schemas.microsoft.com/2003/10/Serialization/Arrays">
//			<c:int>24</c:int>
//			…
//		</b:UnlockedSwitches>
//	</pMessage>
// </SetUnlockedSwitches>
//
// In this application, the body is ignored.
namespace setUnlockedSwitches {
constinit const WS_XML_STRING rootName = u8"SetUnlockedSwitches"_as_xml;

constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/SetUnlockedSwitches"_as_xml;

constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&common::discardedStruct),
};
}



// Constants related to the http://tempuri.org/IWCFRun8/UpdateTrainData action
//
// This action has a body of the following format:
// <UpdateTrainData xmlns="http://tempuri.org/">
//	<pMessage xmlns:b="http://schemas.datacontract.org/2004/07/DispatcherComms.MessagesFromRun8" xmlns:i="http://www.w3.org/2001/XMLSchema-instance">
//		<b:Train>
//			<b:_x003C_AxleCount_x003E_k__BackingField>4</b:_x003C_AxleCount_x003E_k__BackingField>
//			<b:_x003C_BlockID_x003E_k__BackingField>250170</b:_x003C_BlockID_x003E_k__BackingField>
//			<b:_x003C_EngineerName_x003E_k__BackingField></b:_x003C_EngineerName_x003E_k__BackingField>
//			<b:_x003C_EngineerType_x003E_k__BackingField>None</b:_x003C_EngineerType_x003E_k__BackingField>
//			<b:_x003C_HoldingForDispatcher_x003E_k__BackingField>false</b:_x003C_HoldingForDispatcher_x003E_k__BackingField>
//			<b:_x003C_HpPerTon_x003E_k__BackingField>0</b:_x003C_HpPerTon_x003E_k__BackingField>
//			<b:_x003C_LocoNumber_x003E_k__BackingField>292327</b:_x003C_LocoNumber_x003E_k__BackingField>
//			<b:_x003C_RailroadInitials_x003E_k__BackingField>AMTK</b:_x003C_RailroadInitials_x003E_k__BackingField>
//			<b:_x003C_RelinquishWhenStopped_x003E_k__BackingField>false</b:_x003C_RelinquishWhenStopped_x003E_k__BackingField>
//			<b:_x003C_TrainID_x003E_k__BackingField>99991</b:_x003C_TrainID_x003E_k__BackingField>
//			<b:_x003C_TrainLengthFeet_x003E_k__BackingField>74</b:_x003C_TrainLengthFeet_x003E_k__BackingField>
//			<b:_x003C_TrainSpeedLimitMPH_x003E_k__BackingField>0</b:_x003C_TrainSpeedLimitMPH_x003E_k__BackingField>
//			<b:_x003C_TrainSpeedMph_x003E_k__BackingField>0</b:_x003C_TrainSpeedMph_x003E_k__BackingField>
//			<b:_x003C_TrainSymbol_x003E_k__BackingField>None</b:_x003C_TrainSymbol_x003E_k__BackingField>
//			<b:_x003C_TrainWeightTons_x003E_k__BackingField>67</b:_x003C_TrainWeightTons_x003E_k__BackingField>
//		</b:Train>
//	</pMessage>
// </UpdateTrainData>
//
// Note that an array of trains is *not* sent, even per-route; each train is sent as a separate UpdateTrainData action.
namespace updateTrainData {
// Sanity checks on the C++-side structure.
static_assert(std::is_standard_layout_v<TrainData>);
static_assert(std::is_trivial_v<TrainData>);

// The EngineerType enum.
constexpr WS_XML_STRING engineerTypeNoneString = u8"None"_as_xml;
constexpr WS_ENUM_VALUE engineerTypeNone = {
	.value = static_cast<int>(EngineerType::NONE),
	.name = const_cast<WS_XML_STRING *>(&engineerTypeNoneString),
};
constexpr WS_XML_STRING engineerTypePlayerString = u8"Player"_as_xml;
constexpr WS_ENUM_VALUE engineerTypePlayer = {
	.value = static_cast<int>(EngineerType::PLAYER),
	.name = const_cast<WS_XML_STRING *>(&engineerTypePlayerString),
};
constexpr WS_XML_STRING engineerTypeAIString = u8"AI"_as_xml;
constexpr WS_ENUM_VALUE engineerTypeAI = {
	.value = static_cast<int>(EngineerType::AI),
	.name = const_cast<WS_XML_STRING *>(&engineerTypeAIString),
};
constexpr std::array engineerTypeValues = {
	engineerTypeNone,
	engineerTypePlayer,
	engineerTypeAI,
};
constinit const WS_ENUM_DESCRIPTION engineerTypeEnum = {
	.values = const_cast<WS_ENUM_VALUE *>(engineerTypeValues.data()),
	.valueCount = engineerTypeValues.size(),
	.maxByteCount = []() {
		unsigned long ret = 0;
		for(const WS_ENUM_VALUE &i : engineerTypeValues) {
			ret = std::max(ret, i.name->length);
		}
		return ret;
}(),
};

// The _x003C_AxleCount_x003E_k__BackingField XML element.
constinit const WS_XML_STRING axleCountFieldName = u8"_x003C_AxleCount_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION axleCountFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&axleCountFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_UINT32_TYPE,
	.offset = offsetof(TrainData, axleCount),
};

// The _x003C_BlockID_x003E_k__BackingField XML element.
constinit const WS_XML_STRING blockIDFieldName = u8"_x003C_BlockID_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION blockIDFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&blockIDFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_INT32_TYPE,
	.offset = offsetof(TrainData, block),
};

// The _x003C_EngineerName_x003E_k__BackingField XML element.
constinit const WS_XML_STRING engineerNameFieldName = u8"_x003C_EngineerName_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION engineerNameFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&engineerNameFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_WSZ_TYPE,
	.offset = offsetof(TrainData, engineerName),
};

// The _x003C_EngineerType_x003E_k__BackingField XML element.
constinit const WS_XML_STRING engineerTypeFieldName = u8"_x003C_EngineerType_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION engineerTypeFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&engineerTypeFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_ENUM_TYPE,
	.typeDescription = const_cast<WS_ENUM_DESCRIPTION *>(&engineerTypeEnum),
	.offset = offsetof(TrainData, engineerType),
};

// The _x003C_HoldingForDispatcher_x003E_k__BackingField XML element.
constinit const WS_XML_STRING holdingForDispatcherFieldName = u8"_x003C_HoldingForDispatcher_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION holdingForDispatcherFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&holdingForDispatcherFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_BOOL_TYPE,
	.offset = offsetof(TrainData, holdPosition),
};

// The _x003C_HpPerTon_x003E_k__BackingField XML element.
constinit const WS_XML_STRING hpPerTonFieldName = u8"_x003C_HpPerTon_x003E_k__BackingField"_as_xml;
constinit const WS_FLOAT_DESCRIPTION hpPerTonFieldValueDescription = {
	.minValue = 0.0f,
	.maxValue = std::numeric_limits<float>::infinity(),
};
constinit const WS_FIELD_DESCRIPTION hpPerTonFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&hpPerTonFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_FLOAT_TYPE,
	.typeDescription = const_cast<WS_FLOAT_DESCRIPTION *>(&hpPerTonFieldValueDescription),
	.offset = offsetof(TrainData, horsepowerPerTon),
};

// The _x003C_LocoNumber_x003E_k__BackingField XML element.
constinit const WS_XML_STRING locoNumberFieldName = u8"_x003C_LocoNumber_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION locoNumberFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&locoNumberFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_UINT32_TYPE,
	.offset = offsetof(TrainData, locomotiveNumber),
};

// The _x003C_RailroadInitials_x003E_k__BackingField XML element.
constinit const WS_XML_STRING railroadInitialsFieldName = u8"_x003C_RailroadInitials_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION railroadInitialsFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&railroadInitialsFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_WSZ_TYPE,
	.offset = offsetof(TrainData, railroadInitials),
};

// The _x003C_RelinquishWhenStopped_x003E_k__BackingField XML element.
constinit const WS_XML_STRING relinquishWhenStoppedFieldName = u8"_x003C_RelinquishWhenStopped_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION relinquishWhenStoppedFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&relinquishWhenStoppedFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_BOOL_TYPE,
	.offset = offsetof(TrainData, relinquishWhenStopped),
};

// The _x003C_TrainID_x003E_k__BackingField XML element.
constinit const WS_XML_STRING trainIDFieldName = u8"_x003C_TrainID_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION trainIDFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&trainIDFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_UINT32_TYPE,
	.offset = offsetof(TrainData, id),
};

// The _x003C_TrainLengthFeet_x003E_k__BackingField XML element.
constinit const WS_XML_STRING trainLengthFeetFieldName = u8"_x003C_TrainLengthFeet_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION trainLengthFeetFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&trainLengthFeetFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_UINT32_TYPE,
	.offset = offsetof(TrainData, length),
};

// The _x003C_TrainSpeedLimitMPH_x003E_k__BackingField XML element.
constinit const WS_XML_STRING trainSpeedLimitMPHFieldName = u8"_x003C_TrainSpeedLimitMPH_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION trainSpeedLimitMPHFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&trainSpeedLimitMPHFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_UINT32_TYPE,
	.offset = offsetof(TrainData, speedLimit),
};

// The _x003C_TrainSpeedMph_x003E_k__BackingField XML element.
constinit const WS_XML_STRING trainSpeedMPHFieldName = u8"_x003C_TrainSpeedMph_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION trainSpeedMPHFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&trainSpeedMPHFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_FLOAT_TYPE,
	.offset = offsetof(TrainData, speed),
};

// The _x003C_TrainSymbol_x003E_k__BackingField XML element.
constinit const WS_XML_STRING trainSymbolFieldName = u8"_x003C_TrainSymbol_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION trainSymbolFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&trainSymbolFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_WSZ_TYPE,
	.offset = offsetof(TrainData, symbol),
};

// The _x003C_TrainWeightTons_x003E_k__BackingField XML element.
constinit const WS_XML_STRING trainWeightTonsFieldName = u8"_x003C_TrainWeightTons_x003E_k__BackingField"_as_xml;
constinit const WS_FIELD_DESCRIPTION trainWeightTonsFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&trainWeightTonsFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_UINT32_TYPE,
	.offset = offsetof(TrainData, weight),
};

// The Train XML element.
constinit const WS_XML_STRING trainFieldName = u8"Train"_as_xml;
constinit const std::array trainStructFieldDescriptions = {
	const_cast<WS_FIELD_DESCRIPTION *>(&axleCountFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&blockIDFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&engineerNameFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&engineerTypeFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&holdingForDispatcherFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&hpPerTonFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&locoNumberFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&railroadInitialsFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&relinquishWhenStoppedFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&trainIDFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&trainLengthFeetFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&trainSpeedLimitMPHFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&trainSpeedMPHFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&trainSymbolFieldDescription),
	const_cast<WS_FIELD_DESCRIPTION *>(&trainWeightTonsFieldDescription),
};
constinit const WS_STRUCT_DESCRIPTION trainStructDescription = {
	.size = sizeof(TrainData),
	.alignment = alignof(TrainData),
	.fields = const_cast<WS_FIELD_DESCRIPTION **>(trainStructFieldDescriptions.data()),
	.fieldCount = trainStructFieldDescriptions.size(),
};

// The pMessage XML element.
constinit const WS_FIELD_DESCRIPTION trainFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&trainFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::messagesFromRun8),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&trainStructDescription),
};
constinit const std::array pMessageStructFieldDescriptions = {
	const_cast<WS_FIELD_DESCRIPTION *>(&trainFieldDescription),
};
constinit const WS_STRUCT_DESCRIPTION pMessageStructDescription = {
	.size = sizeof(TrainData),
	.alignment = alignof(TrainData),
	.fields = const_cast<WS_FIELD_DESCRIPTION **>(pMessageStructFieldDescriptions.data()),
	.fieldCount = pMessageStructFieldDescriptions.size(),
};

// The root UpdateTrainData XML element.
constinit const WS_FIELD_DESCRIPTION pMessageFieldDescription = {
	.mapping = WS_ELEMENT_FIELD_MAPPING,
	.localName = const_cast<WS_XML_STRING *>(&common::pMessageFieldName),
	.ns = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&pMessageStructDescription),
};
constinit const std::array rootFieldDescriptions = {
	const_cast<WS_FIELD_DESCRIPTION *>(&pMessageFieldDescription),
};
constinit const WS_STRUCT_DESCRIPTION rootStruct = {
	.size = sizeof(TrainData),
	.alignment = alignof(TrainData),
	.fields = const_cast<WS_FIELD_DESCRIPTION **>(rootFieldDescriptions.data()),
	.fieldCount = rootFieldDescriptions.size(),
};
constinit const WS_XML_STRING rootName = u8"UpdateTrainData"_as_xml;
constinit const WS_XML_STRING action = u8"http://tempuri.org/IWCFRun8/UpdateTrainData"_as_xml;
constinit const WS_ELEMENT_DESCRIPTION rootElement = {
	.elementLocalName = const_cast<WS_XML_STRING *>(&rootName),
	.elementNs = const_cast<WS_XML_STRING *>(&common::tempURI),
	.type = WS_STRUCT_TYPE,
	.typeDescription = const_cast<WS_STRUCT_DESCRIPTION *>(&rootStruct),
};
}
}
}

constinit const WS_MESSAGE_DESCRIPTION soap::dispatcherConnectedMessage = {
	.action = const_cast<WS_XML_STRING *>(&dispatcherConnected::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&dispatcherConnected::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::dtmfMessage = {
	.action = const_cast<WS_XML_STRING *>(&dtmf::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&dtmf::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::permissionUpdateMessage = {
	.action = const_cast<WS_XML_STRING *>(&permissionUpdate::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&permissionUpdate::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::radioTextMessage = {
	.action = const_cast<WS_XML_STRING *>(&radioText::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&radioText::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::sendSimulationStateMessage = {
	.action = const_cast<WS_XML_STRING *>(&sendSimulationState::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&sendSimulationState::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::setInterlockErrorSwitchesMessage = {
	.action = const_cast<WS_XML_STRING *>(&setInterlockErrorSwitches::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&setInterlockErrorSwitches::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::setOccupiedBlocksMessage = {
	.action = const_cast<WS_XML_STRING *>(&setOccupiedBlocks::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&setOccupiedBlocks::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::setOccupiedSwitchesMessage = {
	.action = const_cast<WS_XML_STRING *>(&setOccupiedSwitches::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&setOccupiedSwitches::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::setReversedSwitchesMessage = {
	.action = const_cast<WS_XML_STRING *>(&setReversedSwitches::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&setReversedSwitches::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::setSignalsMessage = {
	.action = const_cast<WS_XML_STRING *>(&setSignals::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&setSignals::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::setUnlockedSwitchesMessage = {
	.action = const_cast<WS_XML_STRING *>(&setUnlockedSwitches::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&setUnlockedSwitches::rootElement),
};

constinit const WS_MESSAGE_DESCRIPTION soap::updateTrainDataMessage = {
	.action = const_cast<WS_XML_STRING *>(&updateTrainData::action),
	.bodyElementDescription = const_cast<WS_ELEMENT_DESCRIPTION *>(&updateTrainData::rootElement),
};