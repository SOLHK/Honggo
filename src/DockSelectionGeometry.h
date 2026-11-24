#ifndef HG_DOCK_SELECTION_GEOMETRY_H
#define HG_DOCK_SELECTION_GEOMETRY_H

#include <math.h>
#include <stdbool.h>

typedef struct {
    double x, y, width, height;
} HGADockSelectionBounds;

static inline HGADockSelectionBounds HGADockSelectionBoundsForState(
        double center,double capsuleWidth,double capsuleHeight,double preferredWidth,bool pressed) {
    const double inset=pressed ? 1 : 5;
    const double nearestEdge=fmin(center,capsuleWidth-center);
    const double halfWidth=fmax(0,fmin(preferredWidth*(pressed ? 0.59 : 0.38),nearestEdge-inset));
    const double height=capsuleHeight*(pressed ? 1.08 : 0.66);
    const double lift=pressed ? 2 : 0;
    return (HGADockSelectionBounds){center-halfWidth,(capsuleHeight-height)*0.5-lift,2*halfWidth,height};
}

#endif
