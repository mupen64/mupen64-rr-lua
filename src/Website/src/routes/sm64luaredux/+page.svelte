<script lang="ts">
	import sm64luaredux from '$lib/assets/sm64luaredux.png';
	import Hero from '$lib/components/Hero.svelte';
</script>

<main>
	<Hero
		name="SM64 Lua Redux"
		description="Powerful SM64 TASing utility."
		version="5.1.2"
		logo={sm64luaredux}
		repository="https://github.com/mupen64/SM64LuaRedux"
		bg_color_from="#ffffff"
		bg_color_to="#dddddd"
		dark={false}
	>
		{#snippet button()}
			<a href="https://github.com/mupen64/SM64LuaRedux/releases/latest" class="button primary bg-gray-600! shadow-black! text-white!"
				>Download</a
			>
		{/snippet}
	</Hero>

	<section id="about" class="mb-12 p-16 pb-0">
		<div class="mx-auto max-w-6xl">
			<div class="mb-12 flex flex-row items-center justify-center gap-4 text-3xl font-bold">
				<hr class="w-15 border-2 border-dashed border-red-600" />
				<p>About</p>
				<hr class="w-15 border-2 border-dashed border-red-600" />
			</div>
			<div class="flex flex-col items-center justify-center">
				<p>SM64 Lua Redux is a powerful TASing utility Lua script for Super Mario 64.</p>
			</div>
		</div>
	</section>
</main>
