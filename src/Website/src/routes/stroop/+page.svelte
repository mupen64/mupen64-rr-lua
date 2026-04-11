<script lang="ts">
	import stroop from '$lib/assets/stroop.svg';
	import Hero from '$lib/components/Hero.svelte';
</script>

<main>
	<Hero
		name="STROOP Refactor"
		description="Advanced diagnostic tool for Super Mario 64."
		version="0.7.2"
		logo={stroop}
		repository="https://github.com/FramePerfection/STROOP"
		bg_color_from="#ffffff"
		bg_color_to="#dddddd"
		dark={false}
	></Hero>

	<section id="about" class="mb-12 p-16 pb-0">
		<div class="mx-auto max-w-6xl">
			<div class="mb-12 flex flex-row items-center justify-center gap-4 text-3xl font-bold">
				<hr class="w-15 border-2 border-dashed border-red-600" />
				<p>About</p>
				<hr class="w-15 border-2 border-dashed border-red-600" />
			</div>
			<div class="flex flex-col items-center justify-center">
				<p>STROOP is a powerful diagnostic tool for Super Mario 64.</p>
				<p>
					This fork of STROOP is a refactored version of the original STROOP with better performance
					and new features.
				</p>
			</div>
		</div>
	</section>
</main>
