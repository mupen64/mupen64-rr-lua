<script lang="ts">
	let { children } = $props();
</script>

<div class="mb-12 flex flex-row items-center justify-center gap-4 text-3xl font-bold">
	<hr class="w-full max-w-3xs border border-dashed border-red-600" />
	<h1 class="text-red-600">[</h1>
	<h1 class="whitespace-nowrap">
		{@render children?.()}
	</h1>
	<h1 class="text-red-600">]</h1>
	<hr class="w-full max-w-3xs border border-dashed border-red-600" />
</div>
