<script lang="ts">
	import github from '$lib/assets/github.svg';

	let {
		name,
		description,
		version,
		logo,
		repository,
		button = undefined,
		bg_color_from,
		bg_color_to,
		dark
	} = $props();
</script>

<section
	class="p-16 shadow-md"
	style="background: linear-gradient(45deg, {bg_color_from}, {bg_color_to})"
>
	<div class="mx-auto max-w-6xl">
		<div
			class="flex flex-row flex-wrap items-center justify-center gap-12 {dark
				? 'text-white'
				: 'text-black'}"
		>
			<img src={logo} alt="{name} Logo" class="object-fit w-48" />
			<div class="flex flex-col gap-4">
				<div class="flex flex-col items-center gap-2 text-5xl leading-tight font-bold md:flex-row text-center">
					<p>{name}</p>
					<p
						class="bg-gradient-to-b from-white to-black bg-clip-text text-transparent"
						style="-webkit-text-stroke: 1px white;"
					>
						{version}
					</p>
				</div>
				<p class="text-center text-lg md:text-left">{description}</p>
				<div class="flex flex-wrap items-center justify-center md:justify-start gap-4">
					{#if button}
						{@render button()}
					{/if}
					<a href={repository} class="button">
						<img src={github} alt="Github Logo" class={dark ? 'invert' : ''} />
						<p>See on GitHub</p>
					</a>
				</div>
			</div>
		</div>
	</div>
</section>
