<script lang="ts">
	import '../app.css';
	import mupen64 from '$lib/assets/mupen64.svg';
	import sm64luaredux from '$lib/assets/sm64luaredux.png';
	import ugui from '$lib/assets/ugui.png';
	import { doc_filesystem_to_friendly_name } from '$lib/helpers/DocNameConverter';

	let { children, data } = $props();
</script>

<svelte:head>
	<link rel="icon" href={mupen64} />
</svelte:head>

<div class="bg-surface-2 h-15 flex flex-row items-center justify-center gap-4">
	<a href="/" class="flex flex-row items-center gap-2 py-2 hover:underline">
		<img src={mupen64} alt="Mupen64 Logo" class="object-fit w-7" />
		<span>Mupen64</span>
	</a>

	<div class="group">
		<a href="/docs/win" class=" flex flex-row items-center gap-2 py-2 hover:underline">
			<span>General Docs</span>
		</a>

		<div
			class="bg-surface-light-3 dark:bg-surface-3 absolute hidden flex-col p-2 group-hover:flex"
		>
			{#each data.files as file}
				<a href="/docs/win/{file}" class="flex flex-row items-center gap-2 py-2 hover:underline">
					<span>{doc_filesystem_to_friendly_name(file)}</span>
				</a>
			{/each}
		</div>
	</div>

	<a href="/docs/lua" class="flex flex-row items-center gap-2 py-2 hover:underline">
		<span>Lua Docs</span>
	</a>

	<p class="opacity-70">&middot;</p>
	<a href="/sm64luaredux" class="flex flex-row items-center gap-2 py-2 hover:underline">
		<img src={sm64luaredux} alt="SM64 Lua Redux Logo" class="object-fit w-7" />
		<span>SM64 Lua Redux</span>
	</a>

	<a href="/ugui" class="flex flex-row items-center gap-2 py-2 hover:underline">
		<img src={ugui} alt="ugui Logo" class="object-fit w-7" />
		<span>ugui</span>
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
		<a class="link" href="https://github.com/mupen64/website">Website source on GitHub</a>
		<p>© 2025 Mupen64 Organization</p>
		<p>Licensed under GNU GPL v2 License</p>
	</div>
</footer>
