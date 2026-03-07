
const modules = import.meta.glob('/static/docs/win/*.md', { as: 'raw' });
const docs = await Promise.all(
    Object.entries(modules).map(async ([path, loader]) => ({
        name: path.split('/').pop()?.replace('.md', ''),
        content: await loader()
    }))
);

export async function get_doc_names() {
    return docs.map(doc => doc.name!);
}

export async function get_doc_by_name(name: string) {
    return docs.find(doc => doc.name === name)?.content;
}