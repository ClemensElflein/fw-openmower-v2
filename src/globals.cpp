//
// Created by clemens on 01.10.24.
//

#include "globals.hpp"

CC_SECTION(".ram4") struct board_info board_info {};
CC_SECTION(".ram4") struct carrier_board_info carrier_board_info {};
EVENTSOURCE_DECL(mower_events);
MUTEX_DECL(mower_status_mutex);
// Start with emergency engaged
uint32_t mower_status = MOWER_FLAG_EMERGENCY_LATCH;

void InitGlobals() {
  ID_EEPROM_GetBoardInfo(&board_info);
  ID_EEPROM_GetCarrierBoardInfo(&carrier_board_info);
}
