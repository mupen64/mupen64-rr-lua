<script lang="ts">
	import '../app.css';
	import mupen64 from '$lib/assets/mupen64.svg';
	import sm64luaredux from '$lib/assets/sm64luaredux.png';
	import ugui from '$lib/assets/ugui.png';
	import { doc_name_to_friendly_name } from '$lib/helpers/DocNameConverter';

	let { children, data } = $props();
</script>

<svelte:head>
	<link rel="icon" href={mupen64} />
</svelte:head>

<div
	class="bg-surface-2-light dark:bg-surface-2 h-15 flex flex-row items-center justify-center gap-4 overflow-x-auto overflow-y-hidden whitespace-nowrap"
>
	<div class="group">
		<a href="/" class="flex flex-row items-center gap-2 py-2 hover:underline">
			<img src={mupen64} alt="Mupen64 Logo" class="object-fit w-7" />
			<span class="hidden md:block">Mupen64</span>
		</a>

		<div class="bg-surface-3-light dark:bg-surface-3 absolute hidden flex-col p-2 group-hover:flex">
			<a href="/docs/lua" class="flex flex-row items-center gap-2 py-2 hover:underline">
				<span>Lua Docs</span>
			</a>
			<hr class="border border-dashed border-red-600" />
			{#each data.doc_names as name}
				<a href="/docs/win/{name}" class="flex flex-row items-center gap-2 py-2 hover:underline">
					<span>{doc_name_to_friendly_name(name)}</span>
				</a>
			{/each}
		</div>
	</div>

	<a href="/sm64luaredux" class="flex flex-row items-center gap-2 py-2 hover:underline">
		<img src={sm64luaredux} alt="SM64 Lua Redux Logo" class="object-fit w-7" />
		<span class="hidden md:block">SM64 Lua Redux</span>
	</a>

	<a href="/ugui" class="flex flex-row items-center gap-2 py-2 hover:underline">
		<img src={ugui} alt="ugui Logo" class="object-fit w-7" />
		<span class="hidden md:block">ugui</span>
	</a>
</div>

{@render children?.()}

<footer class="w-full">
	<hr class="border-dashed border-gray-500" />
	<div class="mb-10 mt-10 flex flex-col items-center justify-center text-center">
		<div class="mb-5 flex flex-row items-center gap-2">
			<img src={mupen64} alt="Mupen64 Logo" class="object-fit w-10" />
			<p class="text-2xl font-bold">Mupen64 Organization</p>
		</div>
		<p class="mb-5 max-w-2xl text-center">
			The Mupen64 Organization is a shared-ownership organization maintained by developers around
			the world.
			<br />
			Feel free to contribute!
		</p>

		<a class="link" href="https://github.com/mupen64">© 2025 Mupen64 Organization</a>
		<p>Licensed under GNU GPL v2 License</p>
	</div>
</footer>
