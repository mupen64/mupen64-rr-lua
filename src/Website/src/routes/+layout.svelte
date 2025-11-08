<script lang="ts">
	import '../app.css';
	import mupen64 from '$lib/assets/mupen64.svg';

	let { children } = $props();
</script>

<svelte:head>
	<link rel="icon" href={mupen64} />
</svelte:head>

{@render children?.()}

<footer class="w-full">
	<hr class="border-dashed border-gray-500" />
	<div class="mt-10 mb-10 flex flex-col items-center justify-center text-center">
		<div class="flex flex-row items-center gap-2 mb-5">
			<img src={mupen64} alt="Mupen64 Logo" class="object-fit w-10" />
			<p class="text-2xl font-bold">Mupen64 Organization</p>
		</div>
		<a class="link" href="https://github.com/mupen64/website">Website source on GitHub</a>
		<p>© 2025 Mupen64 Organization</p>
		<p>Licensed under GNU GPL v2 License</p>
	</div>
</footer>
