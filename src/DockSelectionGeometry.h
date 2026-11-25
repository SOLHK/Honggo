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

// The captured UIKit item platter is 62pt high, with 4pt above and below its resting lens.
static inline double HGADockRestingInset(double height) { return height * (4.0 / 62.0); }

static inline HGADockSelectionBounds HGADockSelectionBoundsForState(
        double center,double capsuleWidth,double capsuleHeight,double slotWidth,bool pressed) {
    const double inset=pressed ? 0 : HGADockRestingInset(capsuleHeight);
    // The lifted lens stretches between neighboring tab centers, then rounds
    // out over either glyph. The easing has zero slope at a center and halfway.
    const double distance=pressed ? fabs(remainder(center-slotWidth*0.5,slotWidth))/(slotWidth*0.5) : 0;
    const double stretch=distance*distance*(3-2*distance);
    const double width=fmin(pressed ? slotWidth*(1.12+0.17*stretch) : slotWidth+fmin(2*inset,slotWidth*0.075),capsuleWidth-2*inset);
    const double height=pressed ? capsuleHeight*(1.12-0.075*stretch) : capsuleHeight-2*inset;
    // Slide the lifted lens inward at the ends, retaining its full width.
    const double left=fmax(inset,fmin(center-width*0.5,capsuleWidth-inset-width));
    return (HGADockSelectionBounds){left,(capsuleHeight-height)*0.5,width,height};
}

#endif
