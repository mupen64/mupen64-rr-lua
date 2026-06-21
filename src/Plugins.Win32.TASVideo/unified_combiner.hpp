#pragma once

struct Combiner;

struct UnifiedCompiledCombiner;

void Init_unified_combiner();
void Uninit_unified_combiner();
UnifiedCompiledCombiner *Compile_unified_combiner(Combiner *color, Combiner *alpha);
void Update_unified_combiner_Colors(UnifiedCompiledCombiner *compiled);
void Set_unified_combiner(UnifiedCompiledCombiner *compiled);
void BeginTextureUpdate_unified_combiner();
void EndTextureUpdate_unified_combiner();
