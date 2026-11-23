#pragma once
#import <Foundation/Foundation.h>

// Fields must be plugin-generated technical metadata. Never pass user
// content, URLs, request headers or bodies, credentials or identifiers.
void HGDiagnosticsStart(NSURL *directory, NSDictionary *environment);
void HGDiagnosticsSetForeground(BOOL foreground);
void HGDiagnosticsEnd(void);
void HGLog(NSString *event, NSDictionary *fields);
void HGLogCritical(NSString *event, NSDictionary *fields);
void HGLoadStage(NSString *stage);
BOOL HGEnhancedDiagnostics(void);
void HGSetEnhancedDiagnostics(BOOL enabled);
void HGStoreMaskScene(NSDictionary *scene);

typedef NS_ENUM(NSUInteger, HGDiagnosticScope) {
    HGDiagnosticScopeAll,
    HGDiagnosticScopeSession,
    HGDiagnosticScopePreviousUnexpected,
};

void HGListDiagnosticSessions(void (^completion)(NSArray<NSDictionary *> *, NSError *));
void HGReadDiagnosticSession(NSString *identifier, void (^completion)(NSArray<NSDictionary *> *, NSError *));
void HGExportDiagnosticSessions(HGDiagnosticScope scope, NSString *identifier, NSDictionary *snapshot,
    void (^completion)(NSURL *, NSError *));
void HGExportDiagnostics(NSDictionary *snapshot, void (^completion)(NSURL *, NSError *));
void HGClearDiagnostics(void (^completion)(NSError *));
