<script lang="ts">
	import type { PageProps } from './$types';

	let { data }: PageProps = $props();
</script>

<main>
	<section id="wiki" class="p-16 pb-0">
		<div class="mx-auto max-w-6xl flex flex-col items-center ">
			{@html data.content}
		</div>
	</section>
</main>
