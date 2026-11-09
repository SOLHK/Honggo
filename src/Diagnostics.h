#pragma once
#import <Foundation/Foundation.h>

// Fields are constructed by the plugin: never pass user content, URLs or tokens.
void HGDiagnosticsStart(NSURL *directory, NSDictionary *environment);
void HGLog(NSString *event, NSDictionary *fields);
void HGLoadStage(NSString *stage);
BOOL HGEnhancedDiagnostics(void);
void HGSetEnhancedDiagnostics(BOOL enabled);
void HGExportDiagnostics(NSDictionary *snapshot, void (^completion)(NSURL *, NSError *));
void HGClearDiagnostics(void (^completion)(NSError *));

void HGStoreMaskScene(NSDictionary *scene);
