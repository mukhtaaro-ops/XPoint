#include <PdfHalReflowDocument.h>
#include <PdfPreparation.h>
#include <PdfReflowDocument.h>
#include <PdfTypes.h>

static_assert(sizeof(PdfStatus) > 0, "CrossPDF status type must compile");
static_assert(sizeof(PdfPreparationConfig) > 0, "CrossPDF preparation API must compile");
static_assert(sizeof(PdfReflowDocument) > 0, "CrossPDF reflow document must compile");

void x4ProPlusPdfEngineCompileProbe() {}
