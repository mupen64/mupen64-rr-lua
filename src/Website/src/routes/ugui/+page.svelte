<script lang="ts">
	import ugui from '$lib/assets/ugui.png';
	import Hero from '$lib/components/Hero.svelte';
	import Title from '$lib/components/Title.svelte';
</script>

<main>
	<Hero
		name="ugui"
		description="Flexible immediate-mode Lua GUI library."
		version="3.1.2"
		logo={ugui}
		repository="https://github.com/mupen64/ugui"
		bg_color_from="#0000ff"
		bg_color_to="rgb(39, 17, 209)"
		dark={true}
	></Hero>

	<section id="about" class="mb-12 p-16 pb-0">
		<div class="mx-auto max-w-6xl">
			<Title>About</Title>
			<div class="flex flex-col items-center justify-center">
				<p>
					ugui is a flexible immediate-mode Lua GUI library used by projects in the Mupen64 Lua
					script ecosystem.
				</p>
			</div>
		</div>
	</section>
</main>
