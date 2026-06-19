<script lang="ts">
	import '../app.css';
	import mupen64 from '$lib/assets/mupen64.svg';
	import sm64luaredux from '$lib/assets/sm64luaredux.png';
	import ugui from '$lib/assets/ugui.png';
	import { resolve } from '$app/paths';
	import type { DocNode } from '$lib/helpers/DocFetcher';

	let { children, data } = $props();

	function docHref(node: DocNode): string {
		if (node.is_dir && node.children && node.children.length > 0) {
			// Find the first descendant document
			let current: DocNode = node;
			while (current.is_dir && current.children && current.children.length > 0) {
				current = current.children[0];
			}
			return resolve(`/docs/${current.path}`);
		}
		return resolve(`/docs/${node.path}`);
	}
</script>

{#snippet doc_menu(node: DocNode)}
	{#if node.children && node.children.length > 0}
		{#each node.children as child (child.path)}
			{#if child.is_dir}
				<li>
					<details>
						<summary>{child.title}</summary>
						<ul class="z-1 bg-base-100 p-2">
							{@render doc_menu(child)}
						</ul>
					</details>
				</li>
			{:else}
				<li>
					<a href={docHref(child)}>
						<span>{child.title}</span>
					</a>
				</li>
			{/if}
		{/each}
	{/if}
{/snippet}

<svelte:head>
	<link rel="icon" href={mupen64} />
</svelte:head>

<div class="navbar flex-wrap bg-base-100 shadow-sm sm:flex-nowrap">
	<div class="navbar-start">
		<a class="btn text-xl btn-ghost" href={resolve('/')}>
			<img src={mupen64} alt="Mupen64 Logo" class="object-fit w-8 pr-1" />
			<span>Mupen64 Organization</span>
		</a>
	</div>
	<div class="navbar-center flex">
		<ul class="menu menu-horizontal px-1">
			<li>
				<details>
					<summary>Mupen64</summary>
					<ul class="z-1 w-52 bg-base-100 p-2">
						<li>
							<a href={resolve('/')}>
								<span>Home</span>
							</a>
						</li>

						{@render doc_menu(data.doc_tree)}
					</ul>
				</details>
			</li>

			<li>
				<details>
					<summary>SM64 Lua Redux</summary>
					<ul class="z-1 w-40 bg-base-100 p-2">
						<li>
							<a href={resolve('/sm64luaredux')}>
								<span>Home</span>
							</a>
						</li>
					</ul>
				</details>
			</li>

			<li>
				<details>
					<summary>ugui</summary>
					<ul class="z-1 w-40 bg-base-100 p-2">
						<li>
							<a href={resolve('/ugui')}>
								<span>Home</span>
							</a>
						</li>
					</ul>
				</details>
			</li>

			<li>
				<details>
					<summary>STROOP</summary>
					<ul class="z-1 w-40 bg-base-100 p-2">
						<li>
							<a href={resolve('/stroop')}>
								<span>Home</span>
							</a>
						</li>
					</ul>
				</details>
			</li>
		</ul>
	</div>
	<div class="navbar-end"></div>
</div>

<div class="flex min-h-screen flex-col">
	<div class="flex-1">
		{@render children?.()}
	</div>

	<footer class="grid-300-45 w-full bg-base-200">
		<hr class="border-dashed border-gray-500" />
		<div class="flex flex-col items-center justify-center p-10 text-center">
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

			<a class="link" href="https://github.com/mupen64">© 2026 Mupen64 Organization</a>
			<p>Licensed under GNU GPL v2 License</p>
		</div>
	</footer>
</div>
