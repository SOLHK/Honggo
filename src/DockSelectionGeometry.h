#ifndef HG_DOCK_SELECTION_GEOMETRY_H
#define HG_DOCK_SELECTION_GEOMETRY_H

#include <math.h>
#include <stdbool.h>

typedef struct {
    double x, y, width, height;
} HGADockSelectionBounds;

static inline double HGADockRowWidth(double available,double ratio,double accessory,
                                      unsigned visible,unsigned fullCount,bool stockWidth) {
    const double requested=fmin(available-8,ratio*available);
    // At the stock setting, hiding a tab removes its slot. The remaining
    // glyphs retain the spacing of the complete bar.
    const double slots=stockWidth ? (requested-accessory)*visible/fullCount : requested-accessory;
    return fmin(available-8,accessory+fmax(44*visible,slots));
}

static inline HGADockSelectionBounds HGADockSelectionBoundsForState(
        double center,double capsuleWidth,double capsuleHeight,double slotWidth,bool pressed) {
    const double inset=pressed ? 0 : 1;
    const double width=fmin(slotWidth*(pressed ? 1.12 : 1)-2*inset,capsuleWidth-2*inset);
    const double height=pressed ? capsuleHeight*1.12 : capsuleHeight-2*inset;
    // Slide the lifted lens inward at the ends, retaining its full width.
    const double left=fmax(inset,fmin(center-width*0.5,capsuleWidth-inset-width));
    return (HGADockSelectionBounds){left,(capsuleHeight-height)*0.5,width,height};
}

#endif
