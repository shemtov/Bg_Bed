// =============================================================================
//  comfort  -  the two small comfort modules:
//    autobright  : LDR + time-of-day -> display::setBrightness with smoothing
//    bedselect   : the two LED-strip light buttons pick which bed the panel
//                  controls ("last press wins")
// =============================================================================
#pragma once

namespace autobright {
  void begin();
  void loop();
}

namespace bedselect {
  void begin();
  void loop();
}
