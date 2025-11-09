<script lang="ts">
	import type { PageProps } from './$types';

	let { data }: PageProps = $props();
	console.log(data.content);
</script>

<main>
	<section id="wiki" class="mx-auto max-w-6xl p-16 pb-16">
		<div class="mb-12 flex flex-row items-center justify-center gap-4 text-3xl font-bold">
			<hr class="w-15 border-2 border-dashed border-red-600" />
			<p>{data.title}</p>
			<hr class="w-15 border-2 border-dashed border-red-600" />
		</div>

		<div class="mx-auto flex max-w-4xl flex-col items-start">
			{@html data.content}
		</div>
	</section>
</main>
