# Segfault Analysis for Issue #7

## Problem Description

From the logs provided in issue #7:
```
[MockRuntime] Starting simulation
[MockRuntime] Executing node: "node_start"
[PlayMode] Started/Resumed playback
[MockRuntime] Executing node: "node_dialogue_1"
[MockRuntime] Executing node: "node_choice_1"
[MockRuntime] Executing node: "node_dialogue_2"
[MockRuntime] Executing node: "node_scene_transition"
[MockRuntime] Executing node: "node_dialogue_3"
[MockRuntime] Executing node: "node_end"
[MockRuntime] Reached end of execution
[MockRuntime] Stopped simulation
[PlayMode] Stopped playback
[MockRuntime] Starting simulation
[MockRuntime] Executing node: "node_start"
[PlayMode] Started/Resumed playback
[MockRuntime] Executing node: "node_dialogue_1"
[MockRuntime] Executing node: "node_choice_1"
Segmentation fault (core dumped)
```

**Key Observation**: The segfault happens on the **second** execution of "node_choice_1", not the first.

## Root Cause Analysis

### Location
File: `editor/src/qt/nm_play_mode_controller.cpp:195-234`
Function: `NMPlayModeController::simulateStep()`

### The Bug

Lines 209-214:
```cpp
if (m_currentNodeId.contains("choice")) {
    m_variables["affection"] = m_variables["affection"].toInt() + 10;
} else if (m_currentNodeId.contains("scene")) {
    m_variables["chapter"] = m_variables["chapter"].toInt() + 1;
    m_variables["currentLocation"] = "ParkBench";
}
```

Lines 219-226:
```cpp
if (m_currentNodeId.contains("dialogue")) {
    m_callStack << QString("scene_%1::dialogue_%2")
                       .arg(m_variables["chapter"].toInt())
                       .arg(m_mockStepIndex);
} else if (m_currentNodeId == "node_end") {
    m_callStack.clear();
    m_callStack << "main::start";
}
```

### Why the Second Execution Crashes

1. **First run completes successfully**:
   - Variables are initialized in constructor (line 26-29)
   - First execution of "node_choice_1": affection goes from 50 to 60
   - Execution reaches "node_end"
   - At line 224-225: `m_callStack.clear()` then `m_callStack << "main::start"`

2. **Second run - the problem**:
   - `play()` is called again (line 37-57)
   - Line 44: `m_mockStepIndex = 0` - resets step index
   - Line 45-46: Only resets affection and chapter
   - **Line 47: `m_callStack = {"main::start"};`** - uses initializer list syntax
   - The issue is likely in how `m_callStack` is being manipulated

### Actual Segfault Cause

After closer analysis, the most likely cause is **accessing `m_mockNodeSequence` beyond bounds** or a **signal/slot connection issue**.

Looking at line 204:
```cpp
m_currentNodeId = m_mockNodeSequence[m_mockStepIndex];
```

If `m_mockStepIndex` becomes invalid or `m_mockNodeSequence` is somehow corrupted, this could segfault.

However, the more likely culprit is in lines 219-223 where we build a QString with:
```cpp
m_callStack << QString("scene_%1::dialogue_%2")
                   .arg(m_variables["chapter"].toInt())
                   .arg(m_mockStepIndex);
```

If `m_variables["chapter"]` has been corrupted or becomes an invalid QVariant, the `.toInt()` call could access invalid memory.

### Alternative Theory: Qt Signal Connection

The segfault might also be caused by:
1. `emit currentNodeChanged(m_currentNodeId);` (line 205)
2. `emit variablesChanged(m_variables);` (line 216)
3. `emit callStackChanged(m_callStack);` (line 227)

If any connected slot has been deleted or is accessing invalid memory, this would cause a segfault.

## Solution Strategy

1. **Add defensive checks** before accessing QVariant values
2. **Add verbose logging** to trace execution
3. **Validate** `m_mockStepIndex` bounds
4. **Check** for disconnected or invalid signal connections
5. **Initialize all variables properly** on restart

## Testing Plan

1. Build the editor with added logging
2. Run the play mode twice
3. Observe where the crash occurs with detailed logs
4. Apply fix
5. Verify the fix works
