#pragma once

#if !defined(SOAP_H)
#define SOAP_H

namespace trainlist8 {
namespace soap {
// The DispatcherConnected message.
extern const WS_MESSAGE_DESCRIPTION dispatcherConnectedMessage;

// The possible dispatcher permissions.
enum class DispatcherPermissionLevel : int32_t {
	GRANTED,
	RESCINDED,
	OBSERVER,
};

// The DTMF message, whose body is discarded.
extern const WS_MESSAGE_DESCRIPTION dtmfMessage;

// The body of a DispatcherPermission message.
struct DispatcherPermission final {
	// Whether or not the player has permission to control AI trains.
	BOOL aiPermission;

	// The level of permission the player has over the dispatch board.
	DispatcherPermissionLevel permission;
};

// The PermissionUpdate message, which has a DispatcherPermission struct as its body.
extern const WS_MESSAGE_DESCRIPTION permissionUpdateMessage;

// The RadioText message, whose body is discarded.
extern const WS_MESSAGE_DESCRIPTION radioTextMessage;

// The body of a SendSimulationState message.
struct SimulationState final {
	// Whether this instance of Run 8 is connected to a multiplayer server.
	BOOL client;

	// The current date and time in the simulation.
	WS_DATETIME time;
};

// The SendSimulationState message, which has a SimulationState struct as its body.
extern const WS_MESSAGE_DESCRIPTION sendSimulationStateMessage;

// The SetInterlockErrorSwitches message, whose body is discarded.
extern const WS_MESSAGE_DESCRIPTION setInterlockErrorSwitchesMessage;

// The SetOccupiedBlocks message, whose body is discarded.
extern const WS_MESSAGE_DESCRIPTION setOccupiedBlocksMessage;

// The SetOccupiedSwitches message, whose body is discarded.
extern const WS_MESSAGE_DESCRIPTION setOccupiedSwitchesMessage;

// The SetReversedSwitches message, whose body is discarded.
extern const WS_MESSAGE_DESCRIPTION setReversedSwitchesMessage;

// The SetSignals message, whose body is discarded.
extern const WS_MESSAGE_DESCRIPTION setSignalsMessage;

// The SetUnlockedSwitches message, whose body is discarded.
extern const WS_MESSAGE_DESCRIPTION setUnlockedSwitchesMessage;

// The possible engineer types.
enum class EngineerType : int32_t {
	NONE,
	PLAYER,
	AI,
};

// The body of an UpdateTrainData message.
struct TrainData final {
	// The internal train ID number used to refer to the train in dispatcher protocol messages.
	uint32_t id;

	// The string part of the locomotive’s identifier.
	//
	// For example, for BNSF1234, this field would be BNSF.
	const wchar_t *railroadInitials;

	// The numerical part of the locomotive’s identifier.
	//
	// For example, for BNSF1234, this field would be 1234.
	uint32_t locomotiveNumber;

	// The train’s symbol (aka destination tag).
	const wchar_t *symbol;

	// The number of axles in the train.
	uint32_t axleCount;

	// The HP/t rating of the train.
	float horsepowerPerTon;

	// The length of the train in feet.
	uint32_t length;

	// The train’s speed limit, considering both rolling stock and occupied track, in miles per hour.
	uint32_t speedLimit;

	// The weight of the train’s wagons or carriages, in tons (not including locomotives).
	uint32_t weight;

	// The block the train’s head end currently occupies.
	//
	// This is -1 for trains not in a DS-visible block.
	int32_t block;

	// How fast the train is currently travelling, in miles per hour.
	//
	// This is negative if the controlling locomotive is moving backwards.
	float speed;

	// The name of the human engineer driving the train, or an empty string if the train is uncrewed or driven by AI.
	const wchar_t *engineerName;

	// The type of engineer on board.
	EngineerType engineerType;

	// Whether the AI engineer has been ordered to brake and hold position.
	BOOL holdPosition;

	// Whether the AI engineer has been ordered to disembark when the train next stops.
	BOOL relinquishWhenStopped;
};

// The UpdateTrainData message, which has a TrainData struct as its body.
extern const WS_MESSAGE_DESCRIPTION updateTrainDataMessage;
}
}

#endif