#ifndef HGSiriaiDockMotion_h
#define HGSiriaiDockMotion_h

#include <math.h>
#include <stdint.h>

// 先完成胶囊几何，再过渡到白色，最后才按设置淡出。
static const double DKSiriaiWhiteProgress = 1.16;
static const double DKSiriaiHiddenProgress = 1.34;
static const double DKSiriaiLineWidthRatio = 0.141;
static const double DKSiriaiOpenElasticScale = 0.19;
static const double DKSiriaiCompactElasticScale = 0.30;

typedef struct {
    double width, height, centerX, centerY;
    double cameraScale, cameraX, cameraY;
    double glyphScale, captionAmount, contentAlpha, surfaceAlpha, whiteAmount;
} DKSiriaiDockGeometry;

static inline double DKSiriaiSmoothstep(double from, double to, double value) {
    double t = fmax(0.0, fmin(1.0, (value - from) / (to - from)));
    return t * t * (3.0 - 2.0 * t);
}

static const double DKFeedLandingDuration = 1.0 / 12.0 + 0.22;

// 端点前接住当前速度，只越过一次。到顶与落稳时速度都为零。
static inline double DKFeedLandingSample(double from, double end, double velocity, double elapsed) {
    if (elapsed <= 0.0) return from;
    if (elapsed >= DKFeedLandingDuration) return end;
    const double peakTime = 1.0 / 12.0;
    const double baseReturnTime = 0.20;
    double amplitude = end == 1.0 ? 0.04 : -0.04;
    double peak = end + amplitude;
    if (elapsed < peakTime) {
        // 四次 Bézier 的末两个控制点都在峰值。倒数第三点匹配回程的起始加速度。
        double ratio = peakTime / baseReturnTime;
        double p2 = peak - amplitude * ratio * ratio;
        double p1 = from + velocity * peakTime / 4.0;
        p1 = end == 1.0 ? fmax(from, fmin(p1, p2)) : fmin(from, fmax(p1, p2));
        double u = elapsed / peakTime;
        double r = 1.0 - u;
        return peak + r * r * (r * r * (from - peak) + 4.0 * r * u * (p1 - peak)
                               + 6.0 * u * u * (p2 - peak));
    }
    // 参考片的最后两帧只剩亚像素位移。在原回程里把末端速度降到零，到位即可开始转材质。
    const double finishWindow = 0.035;
    const double returnTime = DKFeedLandingDuration - peakTime;
    double sincePeak = elapsed - peakTime;
    double finishPhase = fmax(0.0, (sincePeak - (returnTime - finishWindow)) / finishWindow);
    double squared = finishPhase * finishPhase;
    sincePeak += finishWindow * squared * squared * (1.0 - finishPhase);

    // 原回程曲线在转向后的前 60ms 保留起步速度。
    const double tailStart = 0.06;
    double x = fmax(0.0, (sincePeak - tailStart) / (returnTime - tailStart));
    double delay = (returnTime - baseReturnTime) * x * x * x * (10.0 + x * (-15.0 + 6.0 * x));
    double u = (sincePeak - delay) / baseReturnTime;
    double remaining = 1.0 - u;
    return end + amplitude * remaining * remaining * remaining * (1.0 + 3.0 * u);
}

// 浅拖留给胶囊和图标收拢，深拖继续压成白条。两段在接点的位置和斜率相同。
static inline double DKSiriaiGeometryProgress(double progress) {
    double p = fmax(0.0, fmin(1.0, progress));
    const double knee = 0.46;
    // 只在最后 4% 的行程匹配整组宽度两侧的斜率。远离端点时修正量及其前两阶导数均为零。
    double edge = fmin(p, 1.0 - p);
    double weight = fmax(0.0, 1.0 - edge / 0.04);
    double correction = edge * weight * weight * weight;
    if (p <= knee) {
        double openSlope = DKSiriaiOpenElasticScale / ((1.0 - DKSiriaiLineWidthRatio) * 1.20);
        return p * (0.34 + 0.16 * p) + (openSlope - 0.34) * correction;
    }
    double atKnee = knee * (0.34 + 0.16 * knee);
    double slope = 0.34 + 0.32 * knee;
    double u = (p - knee) / (1.0 - knee);
    double compactSlope = DKSiriaiLineWidthRatio * DKSiriaiCompactElasticScale
        / ((1.0 - DKSiriaiLineWidthRatio) * 0.80);
    return atKnee + (1.0 - atKnee) * u * u * (3.0 - 2.0 * u)
        + (1.0 - knee) * u * (1.0 - u) * ((1.0 - u) * slope - u * 0.25)
        + (0.25 - compactSlope) * correction;
}

// 每次刷新只提交一次几何。小位移使用临界阻尼，大步输入限制速度；反向从当前显示位置起步。
static inline double DKFeedFollowStep(double shown, double target, double *velocity, double dt, int fingerDown) {
    double gap = target - shown;
    if (gap * *velocity < 0.0) *velocity = 0.0;
    const double frequency = fingerDown ? 58.0 : 65.0;
    const double maxSpeed = 6.0;
    double lag = *velocity - frequency * gap;
    double decay = exp(-frequency * dt);
    double movement = gap + (lag * dt - gap) * decay;
    if (fabs(gap) <= 0.00025 || movement * gap >= gap * gap) {
        *velocity = 0.0;
        return target;
    }
    double speed = (*velocity - frequency * lag * dt) * decay;
    *velocity = fmax(-maxSpeed, fmin(maxSpeed, speed));
    return shown + copysign(fmin(fabs(movement), maxSpeed * dt), gap);
}

// 在已收到的两个位置之间取样，速度延迟和限速单独作用在实际显示的尺寸上。
static inline double DKFeedSampleInput(double current, double target, double dt, double *inputTimeLeft) {
    if (dt <= 0.0) return current;
    if (*inputTimeLeft > dt) target = current + (target - current) * (dt / *inputTimeLeft);
    *inputTimeLeft = fmax(0.0, *inputTimeLeft - dt);
    return target;
}

// 给轻拉保留少量阻尼。换向首帧即移动，大步仍渐近每秒六个行程。
static inline double DKFeedDirectStep(double shown, double target, double *velocity, double dt) {
    if (dt <= 0.0) return shown;
    double gap = target - shown;
    double retained = fmin(0.30, exp(-dt / 0.010)) * gap / (1.0 + fabs(gap) / 0.0075);
    if (fabs(retained) > 0.000001) gap -= retained;
    else retained = 0.0;
    double direct = 5.4 * dt;
    double excess = fabs(gap) - direct;
    if (excess <= 0.0) {
        *velocity = gap / dt;
        return retained == 0.0 ? target : shown + gap;
    }
    double headroom = 0.6 * dt;
    double step = copysign(direct + headroom * excess / (headroom + excess), gap);
    *velocity = step / dt;
    return shown + step;
}

typedef struct {
    double from, velocity, coastTime, duration;
} DKFeedReleaseMotion;

// 刚换向就松手，延续最后的速度，滑行的最远位置限制在已有的回弹范围内。
static inline DKFeedReleaseMotion DKFeedMakeReleaseMotion(double from, double goal, double velocity, double extent) {
    DKFeedReleaseMotion motion = {from, fmax(-6.0, fmin(6.0, velocity)), 0.0, 0.0};
    if (motion.velocity == 0.0) return motion;
    double distance = goal - from;
    double room = motion.velocity > 0.0 ? 1.0 + fmax(0.0, extent) - from : from - fmin(0.0, extent);
    int towardGoal = motion.velocity * distance > 0.0;
    if (towardGoal) room = fabs(distance);
    if (room <= 0.0) return motion;
    motion.duration = 0.12 + 0.15 * sqrt(fmin(1.0, fabs(distance)));
    // 正沿手指方向落向目标时接续实际手速，只在余量很小时提前减速。
    motion.coastTime = fmin(towardGoal ? motion.duration : 0.045, room / fabs(motion.velocity));
    return motion;
}

static inline double DKFeedReleaseSample(DKFeedReleaseMotion motion, double goal, double elapsed) {
    if (elapsed <= 0.0) return motion.from;
    if (elapsed >= motion.duration) return goal;
    double coast = motion.from - motion.velocity * motion.coastTime * expm1(-elapsed / motion.coastTime);
    const double coastLead = 1.0 / 30.0;
    if (elapsed <= coastLead) return coast;
    double u = (elapsed - coastLead) / (motion.duration - coastLead);
    double blend = u * u * u * (10.0 + u * (-15.0 + 6.0 * u));
    return coast + (goal - coast) * blend;
}

// 白色随实际的尺寸变化退去。反向拖动继续退白，松手收到底后再变白。
static inline double DKSiriaiMaterialAfterTravel(double material, double from, double to) {
    double travel = fabs(DKSiriaiGeometryProgress(to) - DKSiriaiGeometryProgress(from));
    return fmax(1.0, material - 4.0 * travel);
}

static inline void DKSiriaiSetDockMaterial(DKSiriaiDockGeometry *shape, double progress) {
    shape->whiteAmount = DKSiriaiSmoothstep(1.0, DKSiriaiWhiteProgress, progress);
    shape->surfaceAlpha = 1.0 - DKSiriaiSmoothstep(DKSiriaiWhiteProgress, DKSiriaiHiddenProgress, progress);
}

// 局部原点在展开时玻璃组的左上角，长条和整个 Dock 共用水平中线。
static inline DKSiriaiDockGeometry DKSiriaiDockShape(double capsuleWidth, double capsuleHeight, double camera,
                                                    double gap, double bottomLimit, double progress) {
    double p = DKSiriaiGeometryProgress(progress);
    double q = p * (1.20 - 0.20 * p);
    double width = capsuleWidth + gap + camera;
    double height = fmax(capsuleHeight, camera);
    double thin = fmin(capsuleHeight, fmax(2.0, fmin(4.0, capsuleHeight * 0.037)));
    double endBottom = fmin(height * 1.14 + thin * 0.5, bottomLimit);
    // 先收紧相邻按钮的间距，初段围绕胶囊中心缩放，后段才移向白条。
    double heightTravel = q * (0.92 + 0.08 * q);
    double groupHeight = height + (thin - height) * heightTravel;
    double center = height * 0.5 + (endBottom - thin * 0.5 - height * 0.5) * heightTravel * q;
    double bottom = center + groupHeight * 0.5;
    DKSiriaiDockGeometry shape;
    shape.width = capsuleWidth + (width * DKSiriaiLineWidthRatio - capsuleWidth) * q;
    shape.height = capsuleHeight + (thin - capsuleHeight) * heightTravel;
    shape.cameraScale = 1.0 - q;
    shape.centerX = (width - (camera + gap) * shape.cameraScale) * 0.5;
    shape.centerY = bottom - shape.height * 0.5;
    shape.cameraX = shape.centerX + shape.width * 0.5 + (gap + camera * 0.5) * shape.cameraScale;
    shape.cameraY = bottom - camera * shape.cameraScale * 0.5;
    // 图标保持等比，先靠拢，再随短条收小；标题在图标仍清晰时退出。
    shape.glyphScale = (1.0 - q) * (1.0 + 0.78 * q);
    shape.captionAmount = 1.0 - DKSiriaiSmoothstep(0.0, 0.20, q);
    shape.contentAlpha = 1.0 - DKSiriaiSmoothstep(0.48, 0.93, p);
    DKSiriaiSetDockMaterial(&shape, progress);
    return shape;
}

// 回弹只变原玻璃组的轮廓，材质从独立的进度读取。拍摄键及其间距算在整组水平中心内。
static inline void DKSiriaiApplyRebound(DKSiriaiDockGeometry *shape, double overtravel, double groupWidth,
                                        double camera, double bottomLimit) {
    if (overtravel == 0.0) return;
    double sx = 1.0 - (overtravel < 0.0 ? DKSiriaiOpenElasticScale : DKSiriaiCompactElasticScale) * overtravel;
    double cameraRadius = camera * shape->cameraScale * 0.5;
    double top = shape->centerY - shape->height * 0.5;
    double bottom = shape->centerY + shape->height * 0.5;
    if (cameraRadius > 0.0) {
        top = fmin(top, shape->cameraY - cameraRadius);
        bottom = fmax(bottom, shape->cameraY + cameraRadius);
    }
    double height = bottom - top;
    double sy = fmin(sx, bottomLimit / height);
    double newTop = fmax(0.0, fmin(bottomLimit - height * sy, top + height * (1.0 - sy) * 0.5));
    double midX = groupWidth * 0.5;
    shape->centerX = midX + (shape->centerX - midX) * sx;
    shape->cameraX = midX + (shape->cameraX - midX) * sx + cameraRadius * (sx - sy);
    shape->centerY = newTop + (shape->centerY - top) * sy;
    shape->cameraY = newTop + (shape->cameraY - top) * sy;
    shape->width *= sx;
    shape->height *= sy;
    shape->cameraScale *= sy;
}

// MARK: - TikTok Compact Geometry, Gesture State Machine & DisplayLink Spring Driver (from DKGlassTabBar.xm & DKTabBarCollapse.xm)

static const double DKFeedCompactWidthRatio = 0.81;
static const double DKFeedCompactHeightRatio = 0.80;
static const double DKFeedLandingRange = 0.12;
static const double DKFeedJitter = 4.0;
static const double DKFeedPullDistance = 18.0;
static const double DKSiriaiSettleDistance = 14.0;
static const double DKFeedFlickProjection = 0.08;
static const double DKFeedVerticalRatio = 1.2;
static const double DKSiriaiWhiteningDuration = 0.14;
static const double DKSiriaiFadeDuration = 0.18;
static const double DKSiriaiFadeDelay = 1.4;
static const double DKFeedFluidMaxElastic = 0.042;
static const double DKFeedFluidMaxDuration = 0.54;

typedef struct {
    int active;
    double goal;
    double a;
    double b;
    double alpha;
    double omegaD;
    double elapsed;
    double maxDuration;
} DKFeedFluidSpring;

// 连续解析欠阻尼弹簧：完整承接松手瞬时速度，轻拉回弹幅度随位移与速度自然缩放，
// 边界外通过 tanh 柔化限幅，保证 30/60/120Hz 下 C2 连续无突跳、无二次抖动。
static inline DKFeedFluidSpring DKFeedMakeFluidSpring(double from, double goal, double velocity) {
    DKFeedFluidSpring sp = {0, goal, 0.0, 0.0, 0.0, 0.0, 0.0, DKFeedFluidMaxDuration};
    double v = fmax(-4.8, fmin(4.8, velocity));
    double y0 = from - goal;
    if (fabs(y0) < 0.0005 && fabs(v) < 0.01) return sp;
    double d = fmin(1.0, fabs(y0));
    // 轻拉回位更利落（omega=17.8, T≈0.35s），跨全行程收缩/展开更舒展灵动（omega=15.8, T≈0.40s）。
    double omega = 17.8 - 2.0 * d;
    const double zeta = 0.76;
    double alpha = zeta * omega;
    double omegaD = omega * sqrt(1.0 - zeta * zeta);
    sp.active = 1;
    sp.goal = goal;
    sp.a = y0;
    sp.b = (v + alpha * y0) / omegaD;
    sp.alpha = alpha;
    sp.omegaD = omegaD;
    sp.elapsed = 0.0;
    sp.maxDuration = DKFeedFluidMaxDuration;
    return sp;
}

static inline double DKFeedFluidSpringSample(const DKFeedFluidSpring *sp, double t,
                                             double *outVelocity, double *outRaw, double *outRawVel) {
    if (!sp || !sp->active || t >= sp->maxDuration) {
        double g = sp ? sp->goal : 0.0;
        if (outVelocity) *outVelocity = 0.0;
        if (outRaw) *outRaw = g;
        if (outRawVel) *outRawVel = 0.0;
        return g;
    }
    if (t <= 0.0) {
        double raw = sp->goal + sp->a;
        double rawVel = sp->omegaD * sp->b - sp->alpha * sp->a;
        if (outRaw) *outRaw = raw;
        if (outRawVel) *outRawVel = rawVel;
        if (raw > 1.0) {
            double th = tanh((raw - 1.0) / DKFeedFluidMaxElastic);
            if (outVelocity) *outVelocity = rawVel * (1.0 - th * th);
            return 1.0 + DKFeedFluidMaxElastic * th;
        }
        if (raw < 0.0) {
            double th = tanh((-raw) / DKFeedFluidMaxElastic);
            if (outVelocity) *outVelocity = rawVel * (1.0 - th * th);
            return -DKFeedFluidMaxElastic * th;
        }
        if (outVelocity) *outVelocity = rawVel;
        return raw;
    }
    double decay = exp(-sp->alpha * t);
    double c = cos(sp->omegaD * t);
    double s = sin(sp->omegaD * t);
    double y = decay * (sp->a * c + sp->b * s);
    double dy = decay * ((sp->omegaD * sp->b - sp->alpha * sp->a) * c
                         - (sp->omegaD * sp->a + sp->alpha * sp->b) * s);
    double raw = sp->goal + y;
    if (outRaw) *outRaw = raw;
    if (outRawVel) *outRawVel = dy;
    if (raw > 1.0) {
        double th = tanh((raw - 1.0) / DKFeedFluidMaxElastic);
        if (outVelocity) *outVelocity = dy * (1.0 - th * th);
        return 1.0 + DKFeedFluidMaxElastic * th;
    }
    if (raw < 0.0) {
        double th = tanh((-raw) / DKFeedFluidMaxElastic);
        if (outVelocity) *outVelocity = dy * (1.0 - th * th);
        return -DKFeedFluidMaxElastic * th;
    }
    if (outVelocity) *outVelocity = dy;
    return raw;
}

static inline double DKFeedMotionTravel(double height, int siriai) {
    return siriai ? fmin(104.0, fmax(52.0, height * 0.12))
                  : fmin(200.0, fmax(84.0, height * 0.22));
}

static inline double DKFeedCompactCapsuleWidth(double capsuleWidth, double cameraDiameter,
                                               double gap, unsigned itemCount) {
    int hasCamera = cameraDiameter > 0.0;
    double group = capsuleWidth + (hasCamera ? gap + cameraDiameter : 0.0);
    double accessory = hasCamera ? gap + cameraDiameter * DKFeedCompactHeightRatio : 0.0;
    double minWidth = fmin(capsuleWidth, (double)itemCount * 44.0);
    return fmax(minWidth, group * DKFeedCompactWidthRatio - accessory);
}

static inline DKSiriaiDockGeometry DKTikTokDockShape(double capsuleX, double capsuleY,
                                                     double capsuleWidth, double capsuleHeight,
                                                     double cameraMidY, double cameraDiameter,
                                                     double gap, unsigned itemCount,
                                                     double progressWithOvertravel) {
    DKSiriaiDockGeometry shape;
    double compactWidth = DKFeedCompactCapsuleWidth(capsuleWidth, cameraDiameter, gap, itemCount);
    double sx = capsuleWidth > 0.0 ? 1.0 + (compactWidth / capsuleWidth - 1.0) * progressWithOvertravel : 1.0;
    double sy = 1.0 - (1.0 - DKFeedCompactHeightRatio) * progressWithOvertravel;
    int hasCamera = cameraDiameter > 0.0;
    double camera = hasCamera ? cameraDiameter : 0.0;
    double usedGap = hasCamera ? gap : 0.0;
    double groupCenterX = capsuleX + (capsuleWidth + usedGap + camera) * 0.5;
    double left = groupCenterX - (capsuleWidth * sx + usedGap + camera * sy) * 0.5;
    double compact = fmin(1.0, fmax(0.0, progressWithOvertravel));

    shape.width = capsuleWidth * sx;
    shape.height = capsuleHeight * sy;
    shape.centerX = left + shape.width * 0.5;
    shape.centerY = capsuleY + capsuleHeight * 0.5;
    shape.cameraScale = sy;
    shape.cameraX = hasCamera ? (left + shape.width + usedGap + camera * sy * 0.5) : shape.centerX;
    shape.cameraY = cameraMidY;
    shape.glyphScale = 1.0 - 0.09 * compact;
    shape.captionAmount = 1.0 - DKSiriaiSmoothstep(0.0, 0.80, compact);
    shape.contentAlpha = 1.0;
    shape.surfaceAlpha = 1.0;
    shape.whiteAmount = 0.0;
    return shape;
}

static inline int DKFeedAllowsHit(double feedProgress, int siriai, int controlsHidden,
                                  double pointX, double pointY,
                                  double capX, double capY, double capW, double capH,
                                  int hasCamera, double camX, double camY, double camW, double camH) {
    if (feedProgress <= 0.005) return 1;
    if (siriai && controlsHidden) return 0;
    int inCapsule = pointX >= capX && pointX <= capX + capW && pointY >= capY && pointY <= capY + capH;
    if (inCapsule) return 1;
    if (hasCamera) {
        return pointX >= camX && pointX <= camX + camW && pointY >= camY && pointY <= camY + camH;
    }
    return 0;
}

typedef enum {
    DKFeedPanIgnored = 0,
    DKFeedPanMoved = 1,
    DKFeedPanFinishedUnlocked = 2,
    DKFeedPanEnded = 3,
} DKFeedPanResult;

typedef struct {
    int active;
    int siriai;
    int direction;
    double lastY;
    double extremeY;
    double travel;
    double motionTravel;
    int caughtMotion;
    double originX;
    double originY;
    int startedCompact;
    int targetCompact;
} DKFeedPanState;

static inline void DKFeedPanReset(DKFeedPanState *state) {
    state->active = 0;
    state->direction = 0;
    state->lastY = 0.0;
    state->extremeY = 0.0;
    state->travel = 0.0;
    state->caughtMotion = 0;
    state->originX = 0.0;
    state->originY = 0.0;
}

static inline void DKFeedPanBegin(DKFeedPanState *state, double viewportHeight, int siriai,
                                  int startedCompact, int caughtMotion,
                                  double initialTransX, double initialTransY, int isBeganState) {
    state->active = 1;
    state->siriai = siriai ? 1 : 0;
    state->direction = 0;
    state->lastY = 0.0;
    state->extremeY = 0.0;
    state->travel = 0.0;
    state->motionTravel = DKFeedMotionTravel(viewportHeight, state->siriai);
    state->caughtMotion = caughtMotion ? 1 : 0;
    state->originX = (state->caughtMotion && isBeganState) ? initialTransX : 0.0;
    state->originY = (state->caughtMotion && isBeganState) ? initialTransY : 0.0;
    state->startedCompact = startedCompact ? 1 : 0;
    state->targetCompact = state->startedCompact;
}

static inline DKFeedPanResult DKFeedPanUpdate(DKFeedPanState *state,
                                              double transX, double transY,
                                              double velocityY, int ended,
                                              double *outDeltaProgress,
                                              int *outEndCompact,
                                              double *outEndVelocityProgress,
                                              int *outLocked) {
    if (outDeltaProgress) *outDeltaProgress = 0.0;
    if (outEndCompact) *outEndCompact = state->startedCompact;
    if (outEndVelocityProgress) *outEndVelocityProgress = 0.0;
    if (outLocked) *outLocked = 0;
    if (!state->active || state->motionTravel <= 0.0) return DKFeedPanIgnored;

    double tx = transX - state->originX;
    double ty = transY - state->originY;
    double y = (ended && state->direction != 0) ? state->lastY : ty;
    if (state->direction == 0) {
        double deadZone = state->caughtMotion ? 0.0 : DKFeedJitter;
        if (fabs(y) < deadZone || fabs(y) <= DKFeedVerticalRatio * fabs(tx)) {
            if (ended) {
                state->active = 0;
                return DKFeedPanFinishedUnlocked;
            }
            return DKFeedPanIgnored;
        }
        state->direction = y < 0.0 ? -1 : 1;
        state->lastY = state->extremeY = (double)state->direction * deadZone;
        state->travel = deadZone;
    }

    if (outLocked) *outLocked = 1;
    double delta = ty - state->lastY;
    state->lastY = ty;
    double progressPerPoint = -1.0 / state->motionTravel;
    if (outDeltaProgress) *outDeltaProgress = delta * progressPerPoint;

    double movement = (y - state->extremeY) * (double)state->direction;
    if (movement <= -DKFeedJitter) {
        state->direction = -state->direction;
        state->travel = DKFeedJitter;
        movement = -movement - DKFeedJitter;
        state->extremeY = y;
    }
    if (movement > 0.0) {
        state->extremeY = y;
        state->travel += movement;
    }

    int towardCompact = state->direction < 0;
    double pullDistance = state->siriai ? DKSiriaiSettleDistance : DKFeedPullDistance;
    double v = ended ? velocityY : 0.0;
    double projected = state->travel + fmax(0.0, (double)state->direction * v * DKFeedFlickProjection);
    if (projected >= pullDistance) {
        state->targetCompact = towardCompact ? 1 : 0;
    }
    if (!ended) return DKFeedPanMoved;

    state->active = 0;
    if (outEndCompact) *outEndCompact = state->targetCompact;
    if (outEndVelocityProgress) *outEndVelocityProgress = v * progressPerPoint;
    return DKFeedPanEnded;
}

typedef enum {
    DKFeedAdvanceKeepRunning = 0,
    DKFeedAdvancePauseLink = 1,
    DKFeedAdvanceFinished = 2,
} DKFeedAdvanceResult;

typedef struct {
    int siriai;
    int compact;
    int following;
    int directTracking;
    int siriaiHidesLine;
    int siriaiFadingLine;
    uint64_t siriaiFadeGeneration;
    double progress;
    double dragStartProgress;
    double siriaiOpeningMaterial;
    double dragTarget;
    double lastDragDelta;
    int dragReversals;
    double lastDragTime;
    double previousInputInterval;
    double inputTimeLeft;
    double sampleProgress;
    DKFeedReleaseMotion releaseMotion;
    double releaseElapsed;
    DKFeedFluidSpring fluidSpring;
    double elasticExtent;
    double lastFrame;
    double dragVelocity;
    double overtravel;
    int lastEndpoint;
    double landingFrom;
    double landingBegan;
    double siriaiMaterialStart;
    double siriaiMaterialBegan;
} DKFeedSpringState;

static inline void DKFeedSpringInit(DKFeedSpringState *s) {
    s->siriai = 0;
    s->compact = 0;
    s->following = 0;
    s->directTracking = 0;
    s->siriaiHidesLine = 0;
    s->siriaiFadingLine = 0;
    s->siriaiFadeGeneration = 0;
    s->progress = 0.0;
    s->dragStartProgress = 0.0;
    s->siriaiOpeningMaterial = 1.0;
    s->dragTarget = 0.0;
    s->lastDragDelta = 0.0;
    s->dragReversals = 0;
    s->lastDragTime = 0.0;
    s->previousInputInterval = 0.0;
    s->inputTimeLeft = 0.0;
    s->sampleProgress = 0.0;
    s->releaseMotion = (DKFeedReleaseMotion){0.0, 0.0, 0.0, 0.0};
    s->releaseElapsed = 0.0;
    s->fluidSpring = (DKFeedFluidSpring){0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, DKFeedFluidMaxDuration};
    s->elasticExtent = 0.0;
    s->lastFrame = 0.0;
    s->dragVelocity = 0.0;
    s->overtravel = 0.0;
    s->lastEndpoint = 0;
    s->landingFrom = 0.0;
    s->landingBegan = 0.0;
    s->siriaiMaterialStart = 1.0;
    s->siriaiMaterialBegan = 0.0;
}

static inline double DKFeedSpringRestTarget(const DKFeedSpringState *s, int compact) {
    if (!compact) return 0.0;
    if (!s->siriai) return 1.0;
    return s->siriaiFadingLine ? DKSiriaiHiddenProgress : DKSiriaiWhiteProgress;
}

static inline double DKFeedSpringDisplayedProgress(const DKFeedSpringState *s) {
    return fmin(1.0, fmax(0.0, s->progress)) + s->overtravel;
}

static inline void DKFeedSpringPickUpSiriaiMaterial(DKFeedSpringState *s) {
    if (s->siriai && s->progress > 1.0) {
        s->siriaiOpeningMaterial = s->progress;
        s->progress = 1.0;
    }
}

static inline void DKFeedSpringCommitProgress(DKFeedSpringState *s, double progress) {
    if (s->siriaiOpeningMaterial > 1.0) {
        s->siriaiOpeningMaterial = DKSiriaiMaterialAfterTravel(s->siriaiOpeningMaterial, s->progress, progress);
    }
    s->progress = progress;
}

static inline void DKFeedSpringFinish(DKFeedSpringState *s, double *outApplyProgress, int *outScheduleFade) {
    s->lastFrame = 0.0;
    s->dragVelocity = 0.0;
    s->releaseMotion.duration = 0.0;
    s->fluidSpring.active = 0;
    s->landingBegan = 0.0;
    s->lastEndpoint = s->compact ? 1 : 0;
    s->overtravel = 0.0;
    s->elasticExtent = 0.0;
    s->siriaiMaterialBegan = 0.0;
    s->siriaiOpeningMaterial = 1.0;
    double target = DKFeedSpringRestTarget(s, s->compact);
    DKFeedSpringCommitProgress(s, target);
    if (outApplyProgress) *outApplyProgress = target;
    if (outScheduleFade) {
        *outScheduleFade = (s->siriai && s->siriaiHidesLine && s->compact && !s->siriaiFadingLine) ? 1 : 0;
    }
}

static inline int DKFeedSpringBeginDrag(DKFeedSpringState *s, int displayLinkRunningUnpaused) {
    s->directTracking = displayLinkRunningUnpaused ? 1 : 0;
    DKFeedSpringPickUpSiriaiMaterial(s);
    s->lastFrame = 0.0;
    s->dragVelocity = 0.0;
    s->landingBegan = 0.0;
    s->siriaiMaterialBegan = 0.0;
    s->siriaiFadingLine = 0;
    s->siriaiFadeGeneration++;
    s->following = 1;
    s->releaseMotion.duration = 0.0;
    s->fluidSpring.active = 0;
    s->lastDragDelta = 0.0;
    s->dragReversals = 0;
    s->previousInputInterval = 0.0;
    s->inputTimeLeft = 0.0;
    s->dragTarget = DKFeedSpringDisplayedProgress(s);
    s->dragStartProgress = s->dragTarget;
    s->sampleProgress = s->dragTarget;
    return s->directTracking;
}

static inline int DKFeedSpringMoveDrag(DKFeedSpringState *s, double delta, double now,
                                       double displayLinkFrame, int displayLinkPaused) {
    if (!s->following || delta == 0.0) return 0;
    double shown = DKFeedSpringDisplayedProgress(s);
    int turned = (delta * s->lastDragDelta) < 0.0;
    int reversed = turned || (delta * (s->dragTarget - shown)) < 0.0;
    s->lastDragDelta = delta;
    if (s->landingBegan > 0.0) {
        if (((s->lastEndpoint == 1) ? delta : -delta) > 0.0) return 0;
        s->landingBegan = 0.0;
        reversed = 1;
    }
    if (reversed) {
        s->dragVelocity = 0.0;
        s->directTracking = 1;
        s->dragTarget = shown;
        s->sampleProgress = shown;
    }
    double lower = fmin(0.0, s->elasticExtent);
    double upper = 1.0 + fmax(0.0, s->elasticExtent);
    double interval = now - s->lastDragTime;
    if (!s->directTracking || reversed) {
        s->previousInputInterval = 0.0;
        s->inputTimeLeft = 0.0;
    } else if (interval > 0.001) {
        int sparse = displayLinkFrame > 0.0 && interval >= 1.5 * displayLinkFrame && interval <= 0.04
            && fabs(interval - s->previousInputInterval) <= 0.5 * displayLinkFrame;
        s->inputTimeLeft = sparse ? interval : 0.0;
        s->previousInputInterval = interval;
    }
    if (interval >= 0.08) {
        s->dragReversals = 0;
        turned = 0;
    }
    s->lastDragTime = now;
    s->dragTarget = fmin(upper, fmax(lower, s->dragTarget + delta));
    if (s->dragTarget != shown) {
        if (turned && s->dragReversals < 2) s->dragReversals++;
        if (displayLinkPaused && now - s->lastFrame >= 0.08) {
            s->lastFrame = 0.0;
            s->dragVelocity = 0.0;
        }
        return 1;
    }
    return 0;
}

static inline void DKFeedSpringSetCompact(DKFeedSpringState *s, int compact, int animated,
                                          double now, int displayLinkActive, int canAnimate,
                                          double *outApplyProgress, int *outFinishedImmediately,
                                          int *outStartLink, int *outScheduleFade) {
    if (outFinishedImmediately) *outFinishedImmediately = 0;
    if (outStartLink) *outStartLink = 0;
    if (outScheduleFade) *outScheduleFade = 0;
    compact = compact ? 1 : 0;
    if (!compact) {
        DKFeedSpringPickUpSiriaiMaterial(s);
    }
    if (s->following || !compact) {
        s->siriaiFadingLine = (compact && s->siriai && s->siriaiHidesLine && s->progress >= 1.0
            && fmax(s->progress, s->siriaiOpeningMaterial) > DKSiriaiWhiteProgress) ? 1 : 0;
        s->siriaiFadeGeneration++;
    }
    double target = DKFeedSpringRestTarget(s, compact);
    if (s->compact == compact && !s->following && animated
        && (displayLinkActive || target == s->progress)) {
        return;
    }
    s->compact = compact;
    s->following = 0;

    if (s->landingBegan > 0.0 && s->lastEndpoint != (compact ? 1 : 0)) {
        s->landingBegan = 0.0;
        s->dragVelocity = 0.0;
    }
    double shown = DKFeedSpringDisplayedProgress(s);
    double goal = compact ? 1.0 : 0.0;
    if (s->fluidSpring.active && s->fluidSpring.goal != goal) {
        s->fluidSpring.active = 0;
    }
    if (animated && canAnimate && !s->fluidSpring.active && s->releaseMotion.duration == 0.0 &&
        s->landingBegan == 0.0 && (shown != goal || s->overtravel != 0.0)) {
        double v0 = (now - s->lastFrame < 0.08) ? s->dragVelocity : 0.0;
        s->fluidSpring = DKFeedMakeFluidSpring(shown, goal, v0);
    }
    if (!animated || !canAnimate
        || (!s->fluidSpring.active && s->releaseMotion.duration == 0.0 && s->landingBegan == 0.0
            && s->overtravel == 0.0 && target == s->progress)) {
        DKFeedSpringFinish(s, outApplyProgress, outScheduleFade);
        if (outFinishedImmediately) *outFinishedImmediately = 1;
        return;
    }
    s->siriaiMaterialBegan = 0.0;
    if (s->landingBegan == 0.0 && !s->fluidSpring.active && now - s->lastFrame >= 0.08) {
        s->dragVelocity = 0.0;
        s->lastFrame = 0.0;
    }
    if (outStartLink) *outStartLink = 1;
}

static inline void DKFeedSpringEndDrag(DKFeedSpringState *s, int compact, double velocity,
                                       double now, double displayLinkFrame,
                                       int displayLinkActive, int canAnimate,
                                       double *outApplyProgress, int *outFinishedImmediately,
                                       int *outStartLink, int *outScheduleFade) {
    if (!s->following) return;
    compact = compact ? 1 : 0;
    double shown = DKFeedSpringDisplayedProgress(s);
    double handoffVel = 0.0;
    int recentDrag = (s->lastDragTime > 0.0 && now - s->lastDragTime < 0.08);
    if (recentDrag) {
        double liveVel = s->dragVelocity;
        if (s->directTracking && s->lastDragDelta != 0.0) {
            double pending = s->dragTarget - s->sampleProgress;
            if (pending * s->lastDragDelta > 0.0) {
                double fallbackFrame = displayLinkFrame > 0.0 ? displayLinkFrame : (1.0 / 60.0);
                double pendingVel = pending / fmax(s->inputTimeLeft, fallbackFrame);
                if (liveVel == 0.0 || fabs(pendingVel) > fabs(liveVel)) liveVel = pendingVel;
            }
        }
        if (liveVel != 0.0 && velocity * liveVel > 0.0) {
            double weight = fabs(velocity) > fabs(liveVel) ? 0.65 : 0.40;
            handoffVel = (1.0 - weight) * liveVel + weight * velocity;
        } else if (liveVel != 0.0) {
            handoffVel = liveVel;
        } else if (s->lastDragDelta != 0.0 && velocity * s->lastDragDelta > 0.0) {
            handoffVel = velocity;
        }
        if (s->directTracking && s->dragReversals >= 2 && fabs(handoffVel) > 0.6) {
            compact = handoffVel > 0.0 ? 1 : 0;
        }
    }
    s->releaseMotion.duration = 0.0;
    s->landingBegan = 0.0;
    if (canAnimate) {
        s->fluidSpring = DKFeedMakeFluidSpring(shown, compact ? 1.0 : 0.0, handoffVel);
    } else {
        s->fluidSpring.active = 0;
    }
    DKFeedSpringSetCompact(s, compact, 1, now, displayLinkActive, canAnimate,
                           outApplyProgress, outFinishedImmediately, outStartLink, outScheduleFade);
}

static inline void DKFeedSpringUpdateHideLine(DKFeedSpringState *s, int hideLine, int displayLinkActive,
                                              int *outStartLink, int *outScheduleFade) {
    if (outStartLink) *outStartLink = 0;
    if (outScheduleFade) *outScheduleFade = 0;
    hideLine = hideLine ? 1 : 0;
    if (hideLine != s->siriaiHidesLine) {
        int wasFading = s->siriaiFadingLine;
        s->siriaiHidesLine = hideLine;
        s->siriaiFadingLine = 0;
        s->siriaiFadeGeneration++;
        if (wasFading && !hideLine && !s->following && s->compact) {
            s->siriaiMaterialBegan = 0.0;
            if (outStartLink) *outStartLink = 1;
        }
        if (!s->following && !displayLinkActive && (!outStartLink || !*outStartLink) && s->compact && hideLine) {
            if (outScheduleFade) *outScheduleFade = 1;
        }
    }
    if (!s->following && !displayLinkActive && (!outStartLink || !*outStartLink)) {
        s->progress = DKFeedSpringRestTarget(s, s->compact);
    }
}

static inline int DKFeedSpringTriggerSiriaiFade(DKFeedSpringState *s, uint64_t ticket) {
    if (ticket != s->siriaiFadeGeneration || s->following || !s->siriai || !s->siriaiHidesLine || !s->compact) {
        return 0;
    }
    s->siriaiFadingLine = 1;
    s->siriaiMaterialBegan = 0.0;
    return 1;
}

static inline DKFeedAdvanceResult DKFeedSpringAdvance(DKFeedSpringState *s,
                                                      double timestamp,
                                                      double targetTimestamp,
                                                      int reduceMotion,
                                                      double *outApplyProgress,
                                                      int *outShouldApply,
                                                      int *outScheduleFade) {
    if (outShouldApply) *outShouldApply = 0;
    if (outScheduleFade) *outScheduleFade = 0;
    double now = targetTimestamp;
    double last = s->lastFrame > 0.0 ? s->lastFrame : timestamp;
    double dt = fmin(1.0 / 30.0, fmax(0.0, now - last));
    s->lastFrame = now;
    double goal = s->following ? s->dragTarget : (s->compact ? 1.0 : 0.0);
    double shown = DKFeedSpringDisplayedProgress(s);

    if (reduceMotion && s->fluidSpring.active) {
        s->fluidSpring.active = 0;
    }

    if (s->following && !s->fluidSpring.active && s->landingBegan == 0.0 && shown != goal) {
        if (fabs(shown - (double)s->lastEndpoint) > DKFeedLandingRange) s->lastEndpoint = -1;
        int endpoint = (goal == 0.0 || goal == 1.0);
        int crossedHalf = fabs(s->dragStartProgress - goal) > 0.5;
        int approaching = s->releaseMotion.duration == 0.0 ||
            (s->releaseElapsed >= 1.0 / 30.0 && (goal - shown) * s->dragVelocity > 0.0);
        if (approaching && crossedHalf && endpoint && goal != (double)s->lastEndpoint && shown >= 0.0 && shown <= 1.0
            && fabs(goal - shown) <= DKFeedLandingRange && !reduceMotion) {
            s->lastEndpoint = (int)goal;
            s->landingFrom = shown;
            s->landingBegan = now - dt;
            s->elasticExtent = 0.0;
            s->releaseMotion.duration = 0.0;
        }
    }

    if (s->fluidSpring.active || s->landingBegan > 0.0 || s->releaseMotion.duration > 0.0 || shown != goal) {
        if (s->fluidSpring.active) {
            s->fluidSpring.elapsed += dt;
            double vel = 0.0, raw = goal, rawVel = 0.0;
            shown = DKFeedFluidSpringSample(&s->fluidSpring, s->fluidSpring.elapsed, &vel, &raw, &rawVel);
            s->dragVelocity = vel;
            if (s->fluidSpring.elapsed >= s->fluidSpring.maxDuration ||
                (fabs(raw - goal) < 0.0009 && fabs(rawVel) < 0.018)) {
                shown = goal;
                s->dragVelocity = 0.0;
                s->fluidSpring.active = 0;
                s->lastEndpoint = (int)goal;
                s->sampleProgress = goal;
                s->inputTimeLeft = 0.0;
                s->elasticExtent = 0.0;
            }
        } else if (s->landingBegan > 0.0) {
            double elapsed = now - s->landingBegan;
            shown = DKFeedLandingSample(s->landingFrom, goal, s->dragVelocity, elapsed);
            if (elapsed >= DKFeedLandingDuration) {
                s->landingBegan = 0.0;
                s->dragVelocity = 0.0;
                s->directTracking = s->following;
                s->sampleProgress = goal;
                s->inputTimeLeft = 0.0;
                s->elasticExtent = 0.0;
            }
        } else if (s->releaseMotion.duration > 0.0) {
            s->releaseElapsed += dt;
            double next = DKFeedReleaseSample(s->releaseMotion, goal, s->releaseElapsed);
            if (dt > 0.0) s->dragVelocity = (next - shown) / dt;
            shown = next;
            if (s->releaseElapsed >= s->releaseMotion.duration) {
                s->releaseMotion.duration = 0.0;
                s->dragVelocity = 0.0;
            }
        } else if (s->following && s->directTracking) {
            s->sampleProgress = DKFeedSampleInput(s->sampleProgress, goal, dt, &s->inputTimeLeft);
            shown = DKFeedDirectStep(shown, s->sampleProgress, &s->dragVelocity, dt);
        } else {
            shown = DKFeedFollowStep(shown, goal, &s->dragVelocity, dt, s->following);
        }
        double geometry = fmin(1.0, fmax(0.0, shown));
        s->overtravel = shown - geometry;
        if (s->overtravel > 0.0) s->elasticExtent = fmax(s->elasticExtent, s->overtravel);
        else if (s->overtravel < 0.0) s->elasticExtent = fmin(s->elasticExtent, s->overtravel);
        else if ((s->elasticExtent > 0.0 && geometry < 1.0 - DKFeedLandingRange)
                 || (s->elasticExtent < 0.0 && geometry > DKFeedLandingRange)) s->elasticExtent = 0.0;
        DKFeedSpringCommitProgress(s, geometry);
        if (outApplyProgress) *outApplyProgress = geometry;
        if (outShouldApply) *outShouldApply = 1;
        if (s->fluidSpring.active || s->landingBegan > 0.0 || s->releaseMotion.duration > 0.0 || shown != goal) {
            return DKFeedAdvanceKeepRunning;
        }
    }

    if (s->following) {
        if (!s->directTracking || timestamp - s->lastDragTime >= 0.05) {
            return DKFeedAdvancePauseLink;
        }
        return DKFeedAdvanceKeepRunning;
    }
    double materialTarget = DKFeedSpringRestTarget(s, s->compact);
    if (materialTarget > 1.0) {
        if (s->siriaiMaterialBegan == 0.0) {
            s->elasticExtent = 0.0;
            s->siriaiMaterialStart = fmax(1.0, fmax(s->progress, s->siriaiOpeningMaterial));
            s->siriaiOpeningMaterial = 1.0;
            s->siriaiMaterialBegan = now;
        }
        double t = (now - s->siriaiMaterialBegan) / (s->siriaiFadingLine ? DKSiriaiFadeDuration : DKSiriaiWhiteningDuration);
        if (t < 1.0 && s->siriaiMaterialStart != materialTarget) {
            double matProgress = s->siriaiMaterialStart + (materialTarget - s->siriaiMaterialStart) * t;
            DKFeedSpringCommitProgress(s, matProgress);
            if (outApplyProgress) *outApplyProgress = matProgress;
            if (outShouldApply) *outShouldApply = 1;
            return DKFeedAdvanceKeepRunning;
        }
    }
    DKFeedSpringFinish(s, outApplyProgress, outScheduleFade);
    if (outShouldApply) *outShouldApply = 1;
    return DKFeedAdvanceFinished;
}

#endif /* HGSiriaiDockMotion_h */
