<script lang="ts">
	import { resolve } from '$app/paths';
	import sm64luaredux from '$lib/assets/sm64luaredux.png';
	import Hero from '$lib/components/Hero.svelte';
	import Title from '$lib/components/Title.svelte';
</script>

<main>
	<Hero
		name="SM64 Lua Redux"
		description="Powerful SM64 TASing utility."
		version="5.3.1"
		logo={sm64luaredux}
		repository="https://github.com/mupen64/SM64LuaRedux"
		bg_color_from="#ffffff"
		bg_color_to="#dddddd"
		dark={false}
	></Hero>

	<div class="pt-16 flex items-center justify-center">
    	<div class="tip">
            <p>
                SM64 Lua Redux shouldn't be downloaded directly.
                <br>
                Get it from the <a class="underline" href={resolve('/')}>repack</a> instead.
            </p>
	    </div>
	</div>

	<section id="about" class="mb-12 p-16 pb-0">
		<div class="mx-auto max-w-6xl">
			<Title>About</Title>
			<div class="flex flex-col items-center justify-center">
				<p>SM64 Lua Redux is a powerful TASing utility Lua script for Super Mario 64.</p>
			</div>
		</div>
	</section>
</main>
