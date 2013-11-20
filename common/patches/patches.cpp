
#include "../debug.h"
#include "patches.h"

#include "Client62.h"
#include "Titanium.h"
#include "Underfoot.h"
#include "SoF.h"
#include "SoD.h"
#include "RoF1.h"
#include "RoF2.h"

void RegisterAllPatches(EQStreamIdentifier &into) {
	Client62::Register(into);
	Titanium::Register(into);
	SoF::Register(into);
	SoD::Register(into);
	Underfoot::Register(into);
	RoF1::Register(into);
	RoF2::Register(into); // not ready (should be remarked out, if not already)
}

void ReloadAllPatches() {
	Client62::Reload();
	Titanium::Reload();
	SoF::Reload();
	SoD::Reload();
	Underfoot::Reload();
	RoF1::Reload();
	RoF2::Reload(); // not ready (should be remarked out, if not already)
}
