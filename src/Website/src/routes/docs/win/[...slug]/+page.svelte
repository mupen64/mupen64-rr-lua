<script lang="ts">
	import Title from '$lib/components/Title.svelte';
	import type { PageProps } from './$types';

	let { data }: PageProps = $props();
</script>

<main>
	<section id="wiki" class="mx-auto max-w-6xl p-4 py-16">
		<Title>{data.title}</Title>

		<div class="mx-auto flex max-w-4xl flex-col items-start">
			{@html data.content}
		</div>
	</section>
</main>
