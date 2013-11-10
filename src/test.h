#pragma once


extern uint32 g_nRunTest;
extern FILE* g_TestOut;
extern FILE* g_TestFailOut;
extern int32 g_nTestFail;
extern int32 g_nTestFalsePositives;

#define QUICK_PERF 0

void WorldInitOcclusionTest();
void StopTest();
void StartTest();
void RunTest(v3& vPos_, v3& vDir_, v3& vRight_);

