<script lang="ts">
	import type { PageProps } from './$types';

	let { data }: PageProps = $props();
</script>

<main>
	<section id="wiki" class="mx-auto max-w-6xl p-4 py-16">
		<div class="mb-12 flex flex-row items-center justify-center gap-4 text-3xl font-bold">
			<hr class="w-full max-w-sm border-2 border-dashed border-red-600" />
			<h1 class="whitespace-nowrap">{data.title}</h1>
			<hr class="w-full max-w-sm border-2 border-dashed border-red-600" />
		</div>

		<div class="mx-auto flex max-w-4xl flex-col items-start">
			{@html data.content}
		</div>
	</section>
</main>
