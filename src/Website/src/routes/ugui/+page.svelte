<script lang="ts">
	import ugui from '$lib/assets/ugui.png';
	import github from '$lib/assets/github.svg';
	import Hero from '$lib/components/Hero.svelte';
</script>

<main>
	<Hero
		name="ugui"
		description="Flexible immediate-mode Lua GUI library."
		version="3.0.0"
		logo={ugui}
		repository="https://github.com/mupen64/ugui"
		bg_color_from="#0000ff"
		bg_color_to="rgb(39, 17, 209)"
		dark={true}
	></Hero>

	<section id="about" class="mb-12 p-16 pb-0">
		<div class="mx-auto max-w-6xl">
			<div class="mb-12 flex flex-row items-center justify-center gap-4 text-3xl font-bold">
				<hr class="w-15 border-2 border-dashed border-red-600" />
				<p>About</p>
				<hr class="w-15 border-2 border-dashed border-red-600" />
			</div>
			<div class="flex flex-col items-center justify-center">
				<p>
					ugui is a flexible immediate-mode Lua GUI library used by projects in the Mupen64 Lua
					script ecosystem.
				</p>
			</div>
		</div>
	</section>
</main>
