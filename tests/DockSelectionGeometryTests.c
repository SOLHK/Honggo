#include "../src/DockSelectionGeometry.h"
#include <assert.h>
#include <stdio.h>

static void near(double actual, double expected) {
    assert(fabs(actual - expected) < 0.00001);
}

int main(void) {
    const double screens[] = {320, 393, 430};
    const double heights[] = {44, 58, 80};
    for (unsigned screen = 0; screen < 3; screen++) {
        for (unsigned count = 1; count <= 4; count++) {
            double total = HGADockRowWidth(screens[screen], 0.90, 66, count, 4, true);
            double width = total - 66;
            double slot = width / count;
            assert(slot >= 44 && total <= screens[screen] - 8);
            near(slot, (screens[screen] * 0.90 - 66) / 4);
            near(HGADockRowWidth(screens[screen], 0.94, 66, count, 4, false), screens[screen] * 0.94);
            for (unsigned h = 0; h < 3; h++) {
                double height = heights[h];
                for (unsigned i = 0; i < count; i++) {
                    double center = (i + 0.5) * slot;
                    HGADockSelectionBounds rest = HGADockSelectionBoundsForState(center,width,height,slot,false);
                    near(rest.x, i * slot + 1);
                    near(rest.x + rest.width, (i + 1) * slot - 1);
                    near(rest.y, 1);
                    near(rest.y + rest.height, height - 1);
                    HGADockSelectionBounds held = HGADockSelectionBoundsForState(center,width,height,slot,true);
                    assert(held.width > rest.width && held.height > rest.height);
                    assert(held.y < 0 && held.y + held.height > height);
                    assert(held.x >= 0 && held.x + held.width <= width + 0.00001);
                    if (i == 0) near(held.x, 0);
                    if (i + 1 == count) near(held.x + held.width, width);
                }
                if (count > 1) {
                    double x = width / 2 - 1;
                    HGADockSelectionBounds drag = HGADockSelectionBoundsForState(x,width,height,slot,true);
                    near(drag.x + drag.width / 2, x);
                }
            }
        }
    }
    puts("Dock row, end coverage and pressed/drag geometry passed");
}
