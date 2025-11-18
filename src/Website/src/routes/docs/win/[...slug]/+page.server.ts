import { Marked } from 'marked';
import type { PageServerLoad } from './$types';
import { redirect } from '@sveltejs/kit';
import { doc_name_to_friendly_name } from '$lib/helpers/DocNameConverter';
import { get_doc_by_name, get_doc_names } from '$lib/helpers/DocFetcher';

export const load: PageServerLoad = async ({ params }) => {

    const doc_names = await get_doc_names();

    if (params.slug == "") {
        redirect(307, `/docs/win/${doc_names[0]}`);
    }

    const content = (await get_doc_by_name(params.slug))!;

    const marked = new Marked({
        hooks: {
            postprocess(html) {
                html = html.replaceAll('[!NOTE]', '<div class="note">');
                html = html.replaceAll('[!NOTE---]', '</div>');
                html = html.replaceAll('[!WARN]', '<div class="warn">');
                html = html.replaceAll('[!WARN---]', '</div>');
                html = html.replaceAll('[!CAUTION]', '<div class="caution">');
                html = html.replaceAll('[!CAUTION---]', '</div>');
                html = html.replaceAll('<p', '<p class="y-2"');
                html = html.replaceAll('<a', '<a class="link"');
                html = html.replaceAll('<h1', '<h1 class="my-4 text-3xl font-bold"');
                html = html.replaceAll('<h2', '<h2 class="my-3 text-2xl"');
                html = html.replaceAll('<h3', '<h3 class="my-2 text-xl"');
                html = html.replaceAll('<code', '<code class="font-mono dark:bg-surface-2 bg-surface-2-light px-1"');
                html = html.replaceAll(
                    '<table',
                    '<table class="my-4 w-full  text-sm rounded-lg overflow-hidden"'
                );
                html = html.replaceAll(
                    '<th',
                    '<th class="bg-surface-2-light dark:bg-surface-2 px-3 py-2 text-left font-semibold"'
                );
                html = html.replaceAll(
                    '<td',
                    '<td class="px-3 py-2 border border-surface-3 dark:border-surface-1"'
                );
                html = html.replaceAll(
                    '<tr',
                    '<tr class="odd:bg-surface-1-light dark:odd:bg-surface-3 even:bg-surface-2-light dark:even:bg-surface-2 transition-colors"'
                );

                return html
            }
        }
    });

    const html = await marked.parse(content);

    return {
        content: html,
        title: doc_name_to_friendly_name(params.slug),
    };
};