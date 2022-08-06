// Generated automatically from src/packages/timer.msc. Do not edit.
static const char* timerModuleSource =
"kabo \"scheduler\" nani DogodaBaga\n"
"\n"
"kulu WaatiMassa {\n"
"  dialen sunogo(milliseconds) {\n"
"    nii (!(milliseconds ye Diat)) Djuru.tike(\"Milliseconds must be a number.\")\n"
"    nii (milliseconds < 0) Djuru.tike(\"Milliseconds cannot be negative.\")\n"
"\n"
"    segin niin DogodaBaga.await_ { WaatiMassa.startTimer_(milliseconds, Djuru.sissanTa) }\n"
"  }\n"
"\n"
"  dunan dialen startTimer_(milliseconds, fiber)\n"
"}\n";
